# Copyright 2024 Sony Semiconductor Solutions Corp. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import contextlib
import datetime
import json
import logging
import random
import re
import shutil
import ssl
import string
import threading
import time
from collections import deque
from collections.abc import Hashable, Mapping
from json import JSONDecodeError
from pathlib import Path
from time import sleep
from typing import Any, ClassVar, TypeVar
from uuid import UUID, uuid4

import allure
import docker
import paho.mqtt.client as mqtt
from allure_commons import _allure
from docker.models.containers import Container
from docker.models.networks import Network
from hamcrest import all_of, assert_that, empty, equal_to, has_entries, has_items, only_contains
from hamcrest.core.matcher import Matcher
from paho.mqtt.client import MQTT_ERR_SUCCESS, MQTTMessage
from retry import retry

from src.ai_models import AIModel
from src.interface import OnWireSchema
from src.modules import Module

K = TypeVar("K", bound=Hashable)


class MQTTBroker:
    _topics: ClassVar = [
        "v1/devices/me/telemetry",
        "v1/devices/me/attributes",
        "v1/devices/me/attributes/request/+",
        "v1/devices/me/attributes/response/+",
        "v1/devices/me/rpc/request/+",
        "v1/devices/me/rpc/response/+",
    ]

    _mqtt_container_name: str = "mqtt"
    _frp_container_name: str = "frp"

    @classmethod
    def mqtt_container_name(cls) -> str:
        return cls._mqtt_container_name

    @classmethod
    def frp_container_name(cls) -> str:
        return cls._frp_container_name

    def __init__(
        self, onwire_schema: OnWireSchema, certificates: Path, enable_device_control_service: bool = False
    ) -> None:
        self._logger: logging.Logger = logging.getLogger(self.__class__.__name__)
        self._onwire_schema = onwire_schema
        self._enable_device_control_service = enable_device_control_service

        self.cafile = certificates / "ca.crt"
        self._certfile = certificates / "pytest.crt"
        self._keyfile = certificates / "pytest.key"

        self._frpc_container: Container | None = None
        self._mqtt_container: Container | None = None

        self._network: Network | None = None

        self._mqttc = mqtt.Client()
        self._dockerc = docker.from_env()

        self._mqttc.on_connect = self._on_connect
        self._mqttc.on_message = self._on_message

        self._messages: dict[str, deque[dict]] = {}

        # A set of hash(topic+payload) of messages
        # being sent by us in order to omit them
        # as receiving messages
        self._published_messages_hash: set[int] = set()

        # It's not required to save all the messages
        # Drop left side message when queue is full (>10)

        for topic in self._topics:
            self._messages[topic] = deque(maxlen=10)

        # An event is required to not block MQTT Thread

        self.is_device_connected = threading.Event()

        self.reqid = 10020

        self.common_cert_dir = Path("src/resources/mqtt-broker/certificates/.current/")

    def _replace_instance(self, instance: str) -> str:
        if self._enable_device_control_service:
            if instance == "backdoor-EA_Main" or instance == "backdoor-EA_UD":
                return "$system"
        return instance

    def _on_connect(self, mqttc: mqtt.Client, obj: Any, flags: dict[str, int], rc: int) -> None:
        for topic in self._topics:
            with _allure.StepContext("Subscribe MQTT Topic", {"topic": topic}):
                self._mqttc.subscribe(topic)

    def _on_message(self, mqttc: mqtt.Client, obj: Any, msg: MQTTMessage) -> None:
        topic = msg._topic.decode()

        message_hash = hash((topic, msg.payload))
        if message_hash not in self._published_messages_hash:
            self._logger.info(f"Received message: Topic [{topic}] with payload [{msg.payload}]")
            if topic not in self._topics:
                topic = topic.rsplit("/", 1)[0] + "/+"

            self._messages[topic].append(json.loads(msg.payload))

            # When a device connects to the broker after being disconnected, it needs a response to initialize.
            # This event will happen at the beginning of the test and if the device reconnects.
            result = re.search(r"^v1\/devices\/([^\/]+)\/attributes\/request\/(\d+)$", msg.topic)
            if result:
                self.publish(f"v1/devices/{result.group(1)}/attributes/response/{result.group(2)}", {})
                self.reqid = int(result.group(2)) + 1
                self.is_device_connected.set()
        else:
            self._logger.debug(
                f"Removing the next message from published by the test. Topic [{topic}] encoded_payload [{msg.payload}] with total hash of [{message_hash}]"  # noqa: E501
            )
            self._published_messages_hash.remove(message_hash)

    def connect(self, host: str) -> None:
        self._mqttc.tls_set(
            ca_certs=str(self.cafile),
            certfile=str(self._certfile),
            keyfile=str(self._keyfile),
            cert_reqs=ssl.CERT_REQUIRED,
        )
        self._mqttc.tls_insecure_set(True)

        self._mqttc.connect(host=host, port=8883)

        self._mqttc.loop_start()

        while not self._mqttc.is_connected():
            sleep(1)

    def _start(self, name: str, tag: str, args: dict, network: str | None = None) -> Container:
        self._dockerc.images.build(
            path=f"src/resources/{tag}",
            rm=True,
            tag=f"{tag}:latest",
            buildargs=args,
        )
        self._logger.info(f"{name} Docker image has been built")

        params = {
            "name": name,
            "image": f"{tag}:latest",
            "detach": True,
        }

        if network:
            container = self._dockerc.containers.run(
                **params,
                network=network,
            )
        else:
            container = self._dockerc.containers.run(
                **params,
                network_mode="host",
            )

        while container.attrs["State"]["Status"] != "running":
            container.reload()
            sleep(1)

        return container

    def start(self, local: bool, frp_host: str, frp_port: int, frp_token: str) -> None:
        self._logger.info("Starting MQTT Broker")
        self.copy_current_mqtt_certs(certificates=self.cafile.parent)
        if local:
            self._mqtt_container = self._start(
                name=self.mqtt_container_name(),
                tag="mqtt-broker",
                args={},
            )

            self.connect("127.0.0.1")

        else:
            self._network = self._dockerc.networks.create(name=self.frp_container_name())
            self._logger.info("FRP network has been created")

            self._mqtt_container = self._start(
                name=self.mqtt_container_name(),
                tag="mqtt-broker",
                args={},
                network=self._network.name,
            )
            self._logger.info("MQTT Container has started")

            self._logger.info("FRP Container starting...")
            self._frpc_container = self._start(
                name=self.frp_container_name(),
                tag="frp-client",
                args={
                    "FRP_HOST": frp_host,
                    "FRP_PORT": str(frp_port),
                    "FRP_TOKEN": frp_token,
                },
                network=self._network.name,
            )
            self._logger.info("FRP Container has started")

            network = self._mqtt_container.attrs["NetworkSettings"]["Networks"][self._network.name]

            self.connect(network["IPAddress"])
            self._logger.info("FRP and MQTT Broker have been connected")

    def _stop(self, container: None | Container, log_file: Path) -> None:
        if container:
            container.stop()
            container.wait()
            log_file.write_bytes(container.logs())
            container.remove()

    def stop(self, logs_folder: Path) -> None:
        self._stop(self._frpc_container, logs_folder / "frpc.logs")
        self._stop(self._mqtt_container, logs_folder / "mqtt.logs")

        if self._network:
            self._network.remove()

        self._dockerc.volumes.prune()

    def clear_messages(self) -> None:
        self._messages = {}

    @allure.step("Publish MQTT Message")
    def publish(self, topic: str, payload: dict) -> None:
        self._logger.info(f"Published message: Topic [{topic}] with payload [{payload}]")
        encoded_payload = json.dumps(payload).encode()
        message_hash = hash((topic, encoded_payload))
        self._logger.debug(
            f"Adding the next message as published by the test. Topic [{topic}] encoded_payload [{encoded_payload!r}] with total hash of [{message_hash}]"  # noqa: E501
        )
        self._published_messages_hash.add(message_hash)
        for _ in range(5):
            rc, _ = self._mqttc.publish(topic, encoded_payload)

            if rc == MQTT_ERR_SUCCESS:
                return

            sleep(1)

        raise ConnectionError

    def wait_mdc_response(self, matcher: Matcher[Mapping[K, Any]], timeout: int, matcher_rpc: Matcher = None) -> dict:
        with _allure.StepContext("Wait RPC Response", {"expected": str(matcher)}):
            start_time = datetime.datetime.fromtimestamp(time.time(), datetime.timezone.utc)
            end_time = start_time + datetime.timedelta(seconds=timeout)
            self._logger.info(f"Started waiting from [{start_time}] until [{end_time}]")
            while start_time < end_time:
                res = self.received_rpc_response
                if res != {}:
                    if matcher_rpc is not None:
                        if matcher_rpc.matches(res):
                            break
                    else:
                        break
                sleep(1)
                start_time = datetime.datetime.fromtimestamp(time.time(), datetime.timezone.utc)

            if not res:
                self._logger.info("Finished waiting unsuccessfully")
                raise TimeoutError(f"MDC Response not received after {timeout}s")

            mdc_response = self._onwire_schema.from_rpc(res)
            assert_that(mdc_response, matcher)
            self._logger.info("Finished waiting successfully")
            return mdc_response

    def wait_stp_request(self, matcher: Matcher[Mapping[K, Any]], timeout: int) -> dict:
        with _allure.StepContext("Wait STP Request", {"expected": str(matcher)}):
            start_time = datetime.datetime.fromtimestamp(time.time(), datetime.timezone.utc)
            end_time = start_time + datetime.timedelta(seconds=timeout)
            while start_time < end_time:
                req = self.received_rpc_request
                if req != {}:
                    try:
                        stp_request = self._onwire_schema.from_stp_request(req)
                        assert_that(stp_request, matcher)
                        return stp_request

                    except Exception:
                        self._logger.error(f"Error with payload {req}", exc_info=True)

                sleep(1)
                start_time = datetime.datetime.fromtimestamp(time.time(), datetime.timezone.utc)

        raise AssertionError(f"Wait STP request failed after {timeout}s")

    def wait_attributes(self, matcher: Matcher[Mapping[K, Any]], timeout: int) -> dict:
        """Wait for an attribute from the device to satisfy the matcher within a given timeout.

        :param matcher: The matcher to satisfy as the expected condition.
        :param timeout: Number of seconds to wait
        :return: The device attributes that has satisfied the matcher

        """
        err_message = None

        with _allure.StepContext("Wait Attributes", {"expected": str(matcher)}):
            start_time = datetime.datetime.fromtimestamp(time.time(), datetime.timezone.utc)
            end_time = start_time + datetime.timedelta(seconds=timeout)
            self._logger.info(f"Started waiting from [{start_time}] until [{end_time}]")
            while start_time < end_time:
                res = self.received_attributes
                try:
                    state: dict = self._onwire_schema.from_config(res)
                    assert_that(state, matcher)
                    self._logger.info("Finished waiting successfully")
                    return state

                except AssertionError as err:
                    err_message = str(err)
                except JSONDecodeError as json_error:
                    self._logger.info(f"Error with payload {res}")
                    err_message = str(json_error)

                sleep(1)
                start_time = datetime.datetime.fromtimestamp(time.time(), datetime.timezone.utc)

        self._logger.info("Finished waiting unsuccessfully")
        raise AssertionError(f"{err_message} after {timeout}s")

    def wait_telemetry(self, matcher: Matcher[Mapping[K, Any]], timeout: int) -> dict:
        """Wait for an telemetry from the device to satisfy the matcher within a given timeout.

        :param matcher: The matcher to satisfy as the expected condition.
        :param timeout: Number of seconds to wait
        :return: The device attributes that has satisfied the matcher

        """
        err_message = None

        with _allure.StepContext("Wait Telemetry", {"expected": str(matcher)}):
            start_time = datetime.datetime.fromtimestamp(time.time(), datetime.timezone.utc)
            end_time = start_time + datetime.timedelta(seconds=timeout)
            self._logger.info(f"Started waiting from [{start_time}] until [{end_time}]")
            while start_time < end_time:
                res = self.received_telemetry
                try:
                    state: dict = self._onwire_schema.from_config(res)
                    self._logger.info(f"received_telemetry state:[{state}] matcher:[{matcher}]")
                    assert_that(state, matcher)
                    self._logger.info("Finished waiting successfully")
                    return state

                except AssertionError as err:
                    err_message = str(err)
                except JSONDecodeError as json_error:
                    self._logger.info(f"Error with payload {res}")
                    err_message = str(json_error)

                sleep(1)
                start_time = datetime.datetime.fromtimestamp(time.time(), datetime.timezone.utc)

        self._logger.info("Finished waiting unsuccessfully")
        raise AssertionError(f"{err_message} after {timeout}s")

    def publish_mdc(self, instance: str, method: str, params: dict, reqid: int | str | None = None) -> str:
        current_reqid = reqid if reqid is not None else self.reqid
        instance = self._replace_instance(instance)
        self.publish(
            f"v1/devices/me/rpc/request/{self.reqid}",
            self._onwire_schema.to_rpc(
                reqid=current_reqid,
                instance=instance,
                method=method,
                params=params,
            ),
        )
        if reqid is None:
            self.reqid += 1
        return current_reqid

    def publish_stp_response(self, url: str, headers: dict[str, str], reqid: int | str) -> None:
        current_reqid = reqid if reqid is not None else self.reqid
        rpc = self._onwire_schema.to_stp_response(reqid=current_reqid, url=url, headers=headers)
        self.publish(
            f"v1/devices/me/rpc/response/{current_reqid}",
            rpc,
        )
        if reqid is None:
            self.reqid += 1

    def publish_configuration(self, instance: str, topic: str, config: dict, reqid: int | str | None = None) -> None:
        instance = self._replace_instance(instance)
        self.publish(
            "v1/devices/me/attributes",
            self._onwire_schema.to_config(
                reqid=reqid if reqid is not None else self.reqid,
                instance=instance,
                topic=topic,
                config=config,
            ),
        )
        if reqid is None:
            self.reqid += 1

    def publish_deployment(self, modules: list[Module], id: UUID | None = None) -> dict:
        modules_to_deploy: list[Module] = []
        if self._enable_device_control_service:
            for module in modules:
                if module.name != "backdoor-EA_Main" and module.name != "backdoor-EA_UD":
                    modules_to_deploy.append(module)
        else:
            modules_to_deploy = modules
        deployment = self._onwire_schema.to_deployment(id or uuid4(), modules_to_deploy)
        self.publish(
            "v1/devices/me/attributes",
            deployment,
        )
        return deployment

    def publish_device_configuration(self, interval_min: int, interval_max: int, configuration_id: UUID) -> None:
        self.publish(
            "v1/devices/me/attributes",
            self._onwire_schema.to_device_config(
                id=configuration_id,
                interval_min=interval_min,
                interval_max=interval_max,
            ),
        )

    def request_deployment(
        self,
        modules: list[Module],
        timeout: int = 60,
        retries: int = 5,
        delay: int = 2,
    ) -> dict:
        @retry(tries=retries, delay=delay)
        def __request_deployment(
            modules: list[Module],
            timeout: int = 60,
        ) -> dict:
            deployment_id = uuid4()

            deployment: dict = self.publish_deployment(
                id=deployment_id,
                modules=modules,
            )

            instance_matchers = {}
            for module in modules:
                instance_id = str(module.instance)
                instance_matchers[instance_id] = has_entries({"status": equal_to("ok")})
            self._logger.info(f"Waiting for deployment with id {deployment_id} to be deployed")
            attributes: dict = self.wait_attributes(
                has_entries(
                    {
                        "deploymentStatus": has_entries(
                            {
                                "reconcileStatus": equal_to("ok"),
                                "deploymentId": equal_to(str(deployment_id)),
                                "instances": has_entries(instance_matchers),
                            }
                        )
                    }
                ),
                timeout,
            )

            for key in attributes["deploymentStatus"]["modules"]:
                assert (
                    attributes["deploymentStatus"]["modules"][key]["status"] == "unknown"
                    or attributes["deploymentStatus"]["modules"][key]["status"] == "ok"
                )
            return deployment

        return __request_deployment(modules=modules, timeout=timeout)

    def request_ai_models(
        self,
        ai_models: list[AIModel],
        backdoor_instance_id: str,
        timeout: int = 10,
        retries: int = 360,
        delay: int = 2,
    ) -> dict:
        """Installs the AI Models to the Device.

        This function can take several minutes to complete due to the low performance of embedded devices when flashing
        AI Models to their storage.

        :param ai_models: List of AI Models to install
        :param backdoor_instance_id: ID of backdoor_EA_Main module
        :param retries: Number of attempts that will be done before considering this a fail
        :param delay: Number of seconds between attempts
        :return: Device attributes
        """

        @retry(tries=retries, delay=delay)
        def __request_ai_models(
            ai_models: list[AIModel],
            backdoor_instance_id: str,
            timeout: int = 10,
        ) -> dict:
            backdoor_instance_id = self._replace_instance(backdoor_instance_id)
            sent_config = self._send_ai_model_request(ai_models, backdoor_instance_id)
            self._logger.info("Waiting for AI Models to be installed. This can take a while")

            attributes: dict = self.wait_attributes(
                all_of(
                    has_entries(
                        {
                            f"state/{backdoor_instance_id}/PRIVATE_deploy_ai_model": has_entries(
                                {
                                    "targets": has_items(
                                        *[
                                            has_entries(
                                                # "progress" and "process_state" are added to each dictionary inside
                                                # "sent_config["targets"]" list of dictionaries to check that all parameters sent
                                                # to the device + "progress" and "process_state" are reported by the device
                                                dict(config, progress=equal_to(100), process_state=equal_to("done"))
                                            )
                                            for config in sent_config["targets"]
                                        ]
                                    ),
                                }
                            ),
                        }
                    ),
                    has_entries(
                        {
                            f"state/{backdoor_instance_id}/device_info": has_entries(
                                {
                                    "ai_models": has_items(
                                        *[has_entries({"version": ai_model.version}) for ai_model in ai_models]
                                    ),
                                }
                            ),
                        }
                    ),
                ),
                timeout * len(ai_models),
            )  # If the AI Model is downloaded from a far away server or the connection is bad, set the above to 4x
            return attributes

        return __request_ai_models(ai_models=ai_models, backdoor_instance_id=backdoor_instance_id, timeout=timeout)

    def request_delete_all_ai_models(
        self,
        backdoor_instance_id: str,
        timeout: int = 60,
        retries: int = 5,
        delay: int = 2,
    ) -> dict:
        """Uninstalls all AI Models in the Device and checks for correct uninstallation.

        :param backdoor_instance_id: ID of backdoor_EA_Main module
        :param retries: Number of attempts that will be done before considering this a fail
        :param delay: Number of seconds between attempts
        :return: Device attributes
        """

        @retry(tries=retries, delay=delay)
        def __request_delete_all_ai_models(
            backdoor_instance_id: str,
            timeout: int = 60,
        ) -> dict:
            backdoor_instance_id = self._replace_instance(backdoor_instance_id)
            self._send_ai_model_request([], backdoor_instance_id)
            self._logger.info("Waiting for AI Models to be uninstalled")

            attributes: dict = self.wait_attributes(
                all_of(
                    has_entries(
                        {
                            f"state/{backdoor_instance_id}/PRIVATE_deploy_ai_model": has_entries(
                                {
                                    "targets": empty(),
                                }
                            ),
                        }
                    ),
                    has_entries(
                        {
                            f"state/{backdoor_instance_id}/device_info": has_entries(
                                {
                                    "ai_models": only_contains(
                                        {
                                            "version": "",
                                            "update_date": "",
                                        },
                                    ),
                                }
                            ),
                        }
                    ),
                ),
                timeout,
            )
            return attributes

        return __request_delete_all_ai_models(backdoor_instance_id=backdoor_instance_id, timeout=timeout)

    def _send_ai_model_request(self, ai_models: list[AIModel], backdoor_instance_id: str) -> dict:
        """Asks for a specific set of AI Models to be present in the Device.

         If this set is empty, the camera will uninstall all AI Models present.

        :param ai_models: List of AI Models that the camera will have
        :param backdoor_instance_id: ID of backdoor_EA_Main module
        :return: Payload sent to the camera for this request
        """
        backdoor_instance_id = self._replace_instance(backdoor_instance_id)

        ai_models_configuration: dict = {"targets": []}
        for ai_model in ai_models:
            ai_models_configuration["targets"].append(
                {
                    "hash": ai_model.hash,
                    "name": ai_model.networkId,
                    "version": ai_model.version,
                    "component": ai_model.component,
                    "package_url": ai_model.host,
                }
            )

        self.publish_configuration(
            backdoor_instance_id,
            "PRIVATE_deploy_ai_model",
            ai_models_configuration,
        )

        return ai_models_configuration

    def request_configuration(
        self,
        instance: str,
        topic: str,
        config: dict,
        matcher: Matcher,
        reqid: int | str | None = None,
        timeout: int = 60,
        retries: int = 5,
        delay: int = 2,
    ) -> dict:
        @retry(tries=retries, delay=delay)
        def __request_configuration(
            instance: str,
            topic: str,
            config: dict,
            matcher: Matcher,
            reqid: int | str | None = None,
            timeout: int = 60,
        ) -> dict:
            instance = self._replace_instance(instance)
            self.publish_configuration(instance, topic, config, reqid)

            return self.wait_attributes(
                matcher,
                timeout,
            )

        return __request_configuration(instance, topic, config, matcher, reqid, timeout)

    def request_mdc(
        self,
        instance: str,
        method: str,
        params: dict,
        matcher: Matcher,
        reqid: int | str | None = None,
        timeout: int = 30,
        retries: int = 5,
        delay: int = 2,
        matcher_rpc: Matcher = None,
    ) -> dict:
        @retry(tries=retries, delay=delay)
        def __request_mdc(
            instance: str,
            method: str,
            params: dict,
            matcher: Matcher,
            reqid: int | str | None = None,
            timeout: int = 30,
        ) -> dict:
            instance = self._replace_instance(instance)
            self.publish_mdc(instance, method, params, reqid)

            return self.wait_mdc_response(matcher, timeout, matcher_rpc)

        return __request_mdc(instance, method, params, matcher, reqid, timeout)

    def set_device_configuration_report_interval(
        self,
        interval_min: int = 3,
        interval_max: int = 180,
        timeout: int = 10,
        retries: int = 2,
        delay: int = 2,
    ) -> dict:
        @retry(tries=retries, delay=delay)
        def __request_device_configuration(
            interval_min: int,
            interval_max: int,
        ) -> dict:
            configuration_id = uuid4()
            self.publish_device_configuration(interval_min, interval_max, configuration_id)

            return self.wait_attributes(
                has_entries(
                    {
                        "state/$agent/report-status-interval-min": equal_to(interval_min),
                        "state/$agent/report-status-interval-max": equal_to(interval_max),
                        "state/$agent/configuration-id": equal_to(str(configuration_id)),
                    }
                ),
                timeout,
            )

        return __request_device_configuration(interval_min, interval_max)

    def _pop_message(self, topic: str) -> dict:
        try:
            message = self._messages[topic].popleft()

            # Syntactic Sugar for Allure
            with _allure.StepContext("Received MQTT Message", {"topic": topic, "payload": json.dumps(message)}):
                return message

        except Exception:
            return {}

    @property
    def received_telemetry(self) -> dict:
        return self._pop_message("v1/devices/me/telemetry")

    @property
    def received_attributes(self) -> dict:
        return self._pop_message("v1/devices/me/attributes")

    @property
    def received_attributes_request(self) -> dict:
        return self._pop_message("v1/devices/me/attributes/request/+")

    @property
    def received_attributes_response(self) -> dict:
        return self._pop_message("v1/devices/me/attributes/response/+")

    @property
    def received_rpc_request(self) -> dict:
        return self._pop_message("v1/devices/me/rpc/request/+")

    @property
    def received_rpc_response(self) -> dict:
        return self._pop_message("v1/devices/me/rpc/response/+")

    def copy_current_mqtt_certs(self, certificates: Path) -> None:
        self._logger.debug(f"Copying certificates from {certificates.absolute()}")
        """
        Creates a directory with a common name with the contents of the directory that has the certificates being used.

        This allows us to avoid having different or cumbersome Dockerfile or mosquitto.conf files for Mosquitto.
        """
        with contextlib.suppress(FileNotFoundError):
            # We only want to ignore errors that consist on the folder not existing. Everything else should raise an
            # exception
            shutil.rmtree(self.common_cert_dir)
        shutil.copytree(certificates, self.common_cert_dir, dirs_exist_ok=True)

    def change_schema(self, version: str) -> None:
        if version not in {"evp1", "evp2"}:
            current = "evp2" if self._enable_device_control_service else "evp1"
            self._logger.error(f"Invalid Version: {version}, Current:{current}")
            return
        self._logger.info(f"Change to {version}")
        self._onwire_schema = OnWireSchema(version)
        if version == "evp2":
            self._enable_device_control_service = True
        else:
            self._enable_device_control_service = False

    def get_schema(self) -> None:
        return "evp2" if self._enable_device_control_service else "evp1"

    def gen_rnd_req_id(len=5) -> str:
        """
        Generate a random string of specified length composed of digits.
        :param len: Length of the random string to generate (default is 5).
        :return: Randomly generated string (Length is len + 1) (first character is always 1 to avoid 0 because first 0 is omitted when converting from string to number).
        """
        return "1" + "".join(random.choices(string.digits, k=len))
