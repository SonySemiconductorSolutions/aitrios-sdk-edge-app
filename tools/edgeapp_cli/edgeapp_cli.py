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

#!/usr/bin/env python3
"""
Simple CLI Tool for AI Model and Edge App Deployment to Raspberry Pi
"""

import argparse
import hashlib
import json
import logging
import os
import readline
import shutil
import socket
import threading
import time
import uuid
from pathlib import Path
from uuid import uuid4
import paho.mqtt.client as mqtt

from src.ai_models import AIModel
from src.modules import Module
from src.interface import OnWireSchema
from colored_logger import get_colored_logger

try:
    from hamcrest import anything, equal_to, has_entries, has_entry
    HAMCREST_AVAILABLE = True
except ImportError:
    HAMCREST_AVAILABLE = False
    print("Warning: hamcrest not available, deployment status checking will be limited")

# Command history configuration
HISTORY_FILE = "./edgeapp_cli_history"

def get_local_ip():
    """Get local IP address for HTTP server"""
    try:
        # Connect to a remote address to determine the local IP
        # This doesn't actually send data, just determines routing
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
            s.connect(("8.8.8.8", 80))
            local_ip = s.getsockname()[0]
            return local_ip
    except Exception:
        # Fallback to localhost if unable to determine IP
        return "localhost"


def load_history():
    """Load command history from file"""
    if os.path.exists(HISTORY_FILE):
        readline.read_history_file(HISTORY_FILE)


def save_history():
    """Save command history to file"""
    readline.write_history_file(HISTORY_FILE)


class SimpleMQTTBroker:
    """Simple MQTT broker wrapper for deployment communication"""

    def __init__(self, host="localhost", port=1883):
        self.host = host
        self.port = port
        # Use Callback API version 2 to avoid deprecation warning
        self.client = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION2)
        self.client.on_connect = self._on_connect
        self.client.on_message = self._on_message
        self.connected = False
        self._logger = get_colored_logger(f"{self.__class__.__name__}")
        self.onwire_schema = OnWireSchema("evp2")

        # Device initialization tracking
        self.device_initialized = False
        self.device_init_event = threading.Event()

        # Message buffering for latest messages
        self.recent_messages = []
        self.max_recent_messages = 3
        self.verbose_logging = False  # Control verbose message logging

        # Deployment status tracking
        self.received_attributes = {}
        self.deployment_status_callbacks = {}
        self.ai_model_status_callbacks = {}

    def broker_log_off(self):
        """Turn off verbose MQTT message logging"""
        self.verbose_logging = False

    def broker_log_on(self):
        """Turn on verbose MQTT message logging"""
        self.verbose_logging = True

    def _on_connect(self, client, userdata, flags, reason_code, properties):
        """Callback API Version 2 - updated signature with reason_code and properties"""
        if reason_code == 0:
            self.connected = True
            self._logger.info("Connected to MQTT broker")
            # Subscribe to relevant topics including device request topics for initialization
            client.subscribe("v1/devices/me/attributes")
            client.subscribe("v1/devices/me/attributes/request/+")  # Critical: Subscribe to device request messages
            client.subscribe("v1/devices/me/telemetry")
            client.subscribe("v1/devices/+/attributes")
            client.subscribe("v1/devices/+/attributes/request/+")  # For multi-device support
            if self.verbose_logging:
                self._logger.info("Subscribed to all necessary MQTT topics including device initialization topics")
        else:
            self._logger.error(f"Failed to connect to MQTT broker (code: {reason_code})")

    def _on_message(self, client, userdata, msg):
        """Callback API Version 2 - message callback (signature unchanged)"""
        try:
            # Decode and parse message
            payload_str = msg.payload.decode()

            # Store recent messages
            message_info = {
                "topic": msg.topic,
                "payload": payload_str,
                "timestamp": time.strftime("%H:%M:%S")
            }

            self.recent_messages.append(message_info)
            if len(self.recent_messages) > self.max_recent_messages:
                self.recent_messages.pop(0)

            # Critical: Device initialization response - when a device connects, it sends request messages
            # that need empty responses to initialize properly (this is missing in original SimpleMQTTBroker)
            import re
            result = re.search(r"^v1\/devices\/([^\/]+)\/attributes\/request\/(\d+)$", msg.topic)
            if result:
                device_id = result.group(1)
                request_id = result.group(2)
                response_topic = f"v1/devices/{device_id}/attributes/response/{request_id}"
                # Send empty response to initialize device
                response_result = self.client.publish(response_topic, json.dumps({}))
                # Always log device initialization for debugging
                self._logger.info(f"Device initialization: Responded to {msg.topic} with empty response on {response_topic} (rc={response_result.rc})")
                if response_result.rc == 0:
                    self._logger.debug(f"Successfully sent device initialization response")
                    # Mark device as initialized after successful response
                    if not self.device_initialized:
                        self.device_initialized = True
                        self.device_init_event.set()
                        self._logger.debug("Device initialization completed - CLI ready to accept commands")
                else:
                    self._logger.warning(f"Failed to send device initialization response: {response_result.rc}")

            # Alternative device initialization by telemetry message
            # If device sends telemetry messages, consider it as connected
            if msg.topic == "v1/devices/me/telemetry":
                if not self.device_initialized:
                    self.device_initialized = True
                    self.device_init_event.set()
                    self._logger.info(f"Device initialization: Marked as complete by telemetry message from {msg.topic}")

            # Parse JSON payload and store attributes
            try:
                payload_data = json.loads(payload_str)
                if isinstance(payload_data, dict):
                    self.received_attributes.update(payload_data)

                    # Check if this is an AI model deployment message and log it
                    has_ai_model_msg = any(("PRIVATE_deploy_ai_model" in key) for key in payload_data.keys())
                    if has_ai_model_msg and self.verbose_logging:
                        self._logger.info(f"AI Model Deployment Message: {payload_str}")

                    # Check for deployment status updates
                    self._check_deployment_status(payload_data)
                    self._check_ai_model_status(payload_data)

            except json.JSONDecodeError:
                pass  # Ignore non-JSON messages

            # Only log if verbose logging is enabled
            if self.verbose_logging:
                self._logger.info(f"Received message on {msg.topic}: {payload_str}")

        except Exception as e:
            self._logger.error(f"Error processing message: {e}")

    def _check_deployment_status(self, payload_data):
        """Check for deployment status updates and trigger callbacks"""
        if "deploymentStatus" in payload_data:
            deployment_status = payload_data["deploymentStatus"]
            deployment_id = deployment_status.get("deploymentId")

            if deployment_id and deployment_id in self.deployment_status_callbacks:
                callback = self.deployment_status_callbacks[deployment_id]
                try:
                    callback(deployment_status)
                except Exception as e:
                    self._logger.error(f"Error in deployment status callback: {e}")

    def _check_ai_model_status(self, payload_data):
        """Check for AI model deployment status updates"""
        # Look for state updates related to AI model deployment
        for key, value in payload_data.items():
            if (key.startswith("state/") and "PRIVATE_deploy_ai_model" in key) or \
               (key.startswith("configuration/") and "PRIVATE_deploy_ai_model" in key):
                # Parse the state/configuration message
                try:
                    if isinstance(value, str):
                        state_data = json.loads(value)
                    else:
                        state_data = value

                    # Check for progress updates
                    self._check_ai_model_progress(state_data)

                    if "res_info" in state_data:
                        res_info = state_data["res_info"]
                        res_id = res_info.get("res_id")

                        # Check all active callbacks, not just exact req_id match
                        # This handles cases where req_id might be encoded differently
                        if self.ai_model_status_callbacks:
                            self._logger.debug(f"Processing AI model status for res_id: {res_id}")
                            self._logger.debug(f"Active callbacks: {list(self.ai_model_status_callbacks.keys())}")

                        for callback_req_id, callback in list(self.ai_model_status_callbacks.items()):
                            try:
                                callback(res_info, state_data)
                            except Exception as e:
                                self._logger.error(f"Error in AI model status callback: {e}")

                except (json.JSONDecodeError, TypeError) as e:
                    if self.verbose_logging:
                        self._logger.warning(f"Failed to parse AI model message for key {key}: {e}")
                        self._logger.debug(f"Message value: {value}")
                except Exception as e:
                    self._logger.error(f"Unexpected error processing AI model status: {e}")

    def _check_ai_model_progress(self, state_data):
        """Check and log AI model deployment progress"""
        if "targets" in state_data:
            targets = state_data["targets"]
            for target in targets:
                if isinstance(target, dict):
                    progress = target.get("progress", 0)
                    process_state = target.get("process_state", "unknown")
                    chip = target.get("chip", "unknown")

                    if progress is not None and process_state:
                        if self.verbose_logging:
                            self._logger.debug(f"AI Model Progress - Chip: {chip}, Progress: {progress}%, State: {process_state}")

    def show_recent_messages(self):
        """Show the last few received messages"""
        if self.recent_messages:
            self._logger.info("=== Recent MQTT Messages ===")
            for msg in self.recent_messages:
                self._logger.info(f"[{msg['timestamp']}] {msg['topic']}: {msg['payload']}")
        else:
            # Display "No recent messages" in red to make it more noticeable
            self._logger.error("No recent messages")

    def connect(self):
        """Connect to MQTT broker"""
        try:
            self._logger.info(f"Connecting to MQTT broker at {self.host}:{self.port}")
            self.client.connect(self.host, self.port, 60)
            self.client.loop_start()

            # Wait for connection
            timeout = 10
            while timeout > 0 and not self.connected:
                time.sleep(0.5)
                timeout -= 0.5

            if not self.connected:
                self._logger.error("Failed to connect to MQTT broker within timeout")
                return False

            return True
        except Exception as e:
            self._logger.error(f"Error connecting to MQTT broker: {e}")
            return False

    def wait_for_device_initialization(self, timeout=60):
        """Wait for device to send initialization request and receive response"""
        if self.device_initialized:
            self._logger.debug("Device already initialized")
            return True

        self._logger.debug(f"Waiting for device initialization (timeout: {timeout}s)...")
        self._logger.debug("Device needs to send initialization request to v1/devices/me/attributes/request/{id}")

        if self.device_init_event.wait(timeout=timeout):
            self._logger.debug("Device initialization completed successfully")
            return True
        else:
            self._logger.warning(f"Device initialization timeout after {timeout}s")
            self._logger.warning("Device may not be connected or not sending initialization requests")
            return False

    def disconnect(self):
        """Disconnect from MQTT broker"""
        if self.connected:
            self.client.loop_stop()
            self.client.disconnect()
            self.connected = False
            self._logger.info("Disconnected from MQTT broker")

    def publish_deployment(self, modules):
        """Publish deployment message"""
        deployment_id = uuid4()
        deployment = self.onwire_schema.to_deployment(deployment_id, modules)

        topic = "v1/devices/me/attributes"
        message = json.dumps(deployment)

        result = self.client.publish(topic, message)
        if result.rc == mqtt.MQTT_ERR_SUCCESS:
            self._logger.info(f"Published deployment: {deployment_id}")
            return True
        else:
            self._logger.error(f"Failed to publish deployment: {result.rc}")
            return False

    def publish_ai_model_config(self, ai_models, instance="$system"):
        """Publish AI model configuration"""
        config = {
            "ai_models": [
                {
                    "name": model.name,
                    "url": model.url,
                    "hash": model.hash,
                    "version": model.version
                } for model in ai_models
            ]
        }

        message = self.onwire_schema.to_config(
            reqid=str(uuid4()),
            instance=instance,
            topic="ai_model_deploy",
            config=config
        )

        topic = "v1/devices/me/attributes"
        result = self.client.publish(topic, json.dumps(message))

        if result.rc == mqtt.MQTT_ERR_SUCCESS:
            self._logger.info(f"Published AI model configuration for {len(ai_models)} models")
            return True
        else:
            self._logger.error(f"Failed to publish AI model configuration: {result.rc}")
            return False

    def wait_for_deployment_status(self, deployment_id, timeout=60):
        """Wait for deployment status with specified deployment ID"""
        deployment_success = threading.Event()
        deployment_result = {"status": None, "error": None}

        def status_callback(deployment_status):
            reconcile_status = deployment_status.get("reconcileStatus")
            if reconcile_status == "ok":
                deployment_result["status"] = "success"
                deployment_success.set()
            elif reconcile_status in ["error", "failed"]:
                deployment_result["status"] = "failed"
                deployment_result["error"] = deployment_status.get("message", "Unknown error")
                deployment_success.set()

        # Register callback
        self.deployment_status_callbacks[deployment_id] = status_callback

        try:
            # Wait for deployment completion with keyboard interrupt handling
            start_time = time.time()
            while not deployment_success.is_set():
                try:
                    if deployment_success.wait(timeout=1):  # Check every second
                        break

                    # Check timeout
                    elapsed = time.time() - start_time
                    if elapsed >= timeout:
                        return False, f"Timeout after {timeout} seconds"

                except KeyboardInterrupt:
                    # Allow keyboard interrupt to break the wait loop
                    return False, "Deployment cancelled by user"

            return deployment_result["status"] == "success", deployment_result.get("error")
        finally:
            # Clean up callback
            self.deployment_status_callbacks.pop(deployment_id, None)

    def wait_for_ai_model_config_response(self, req_id, timeout=60):
        """Wait for AI model configuration response with progress monitoring"""
        config_success = threading.Event()
        config_result = {"status": None, "error": None, "progress": 0}
        last_progress = -1

        def config_callback(res_info, state_data=None):
            nonlocal last_progress

            # Check for progress updates in state_data
            if state_data and "targets" in state_data:
                targets = state_data["targets"]
                for target in targets:
                    if isinstance(target, dict):
                        progress = target.get("progress", 0)
                        process_state = target.get("process_state", "unknown")
                        chip = target.get("chip", "unknown")

                        if progress != last_progress:
                            config_result["progress"] = progress
                            last_progress = progress
                            self._logger.debug(f"AI Model Deployment Progress: {progress}% ({process_state}) - Chip: {chip}")

                        # Check if deployment is complete - more flexible success condition
                        if progress >= 100 and process_state == "done":
                            config_result["status"] = "success"
                            config_success.set()
                            return
                        # Check for failure states
                        elif process_state and process_state.startswith("failed"):
                            config_result["status"] = "failed"
                            config_result["error"] = f"Deployment failed: {process_state}"
                            config_success.set()
                            return
                        # Also check for successful response code with high progress
                        elif progress >= 100 and res_info.get("code") == 0:
                            config_result["status"] = "success"
                            config_success.set()
                            return

            # Check response info for errors and success
            code = res_info.get("code")
            if code == 0:  # Success
                # If we have high progress or explicit success, mark as complete
                if config_result.get("progress", 0) >= 100:
                    config_result["status"] = "success"
                    config_success.set()
                # Don't set success here if progress is low, wait for progress to reach 100%
            elif code is not None and code != 0:
                config_result["status"] = "failed"
                detail_msg = res_info.get("detail_msg", f"Error code: {code}")
                config_result["error"] = detail_msg
                config_success.set()  # Exit immediately on error
                config_success.set()

        # Register callback
        self.ai_model_status_callbacks[req_id] = config_callback
        self._logger.info(f"Registered AI model callback with req_id: {req_id}")

        try:
            # Wait for configuration completion with keyboard interrupt handling
            start_time = time.time()
            while not config_success.is_set():
                try:
                    if config_success.wait(timeout=1):  # Check every second
                        break

                    # Check timeout
                    elapsed = time.time() - start_time
                    if elapsed >= timeout:
                        # Before timing out, check one more time if we missed a success
                        if config_result.get("progress", 0) >= 100:
                            self._logger.info("Detected completion on timeout check")
                            return True, None
                        return False, f"Timeout after {timeout} seconds (last progress: {config_result['progress']}%)"

                except KeyboardInterrupt:
                    # Allow keyboard interrupt to break the wait loop
                    return False, "Deployment cancelled by user"

            return config_result["status"] == "success", config_result.get("error")
        finally:
            # Clean up callback
            self.ai_model_status_callbacks.pop(req_id, None)

    def send_configuration(self, config_payload, instance, topic=None):
        """Send configuration to device via MQTT"""
        try:
            # If topic is not specified, get the first key from the payload
            if topic is None:
                if isinstance(config_payload, dict) and len(config_payload) > 0:
                    topic = list(config_payload.keys())[0]
                    config = config_payload[topic]
                else:
                    raise ValueError("Cannot determine topic from configuration payload")
            else:
                config = config_payload

            # Extract the actual configuration content for edge_app
            if topic == "edge_app" and isinstance(config, dict) and "edge_app" in config:
                # Extract the inner configuration content
                inner_config = config["edge_app"]
                self._logger.debug("Extracted inner edge_app configuration to match successful pattern")
                config = inner_config

            # Generate request ID for tracking
            import random
            import string
            req_id = "".join(random.choices(string.ascii_letters + string.digits, k=15))

            # Create state topic for response monitoring
            state_topic = f"state/{instance}/{topic}"

            self._logger.info(f"Sending configuration to instance '{instance}' with topic '{topic}'")
            self._logger.info(f"Request ID: {req_id}")
            self._logger.debug(f"State topic: {state_topic}")

            # Use hamcrest matchers if available
            if HAMCREST_AVAILABLE:
                response_matcher = has_entry(state_topic, has_entry("res_info", has_entry("res_id", equal_to(req_id))))
            else:
                response_matcher = None

            # Create configuration message using OnWire schema
            message = self.onwire_schema.to_config(
                reqid=req_id,
                instance=instance,
                topic=topic,
                config=config
            )

            # Publish configuration
            mqtt_topic = "v1/devices/me/attributes"
            result = self.client.publish(mqtt_topic, json.dumps(message))

            if result.rc != mqtt.MQTT_ERR_SUCCESS:
                self._logger.error(f"Failed to publish configuration: {result.rc}")
                return False, f"MQTT publish failed: {result.rc}"

            self._logger.info(f"Configuration published successfully")

            # Wait for response if hamcrest is available and we have the required infrastructure
            if HAMCREST_AVAILABLE and hasattr(self, '_broker') and hasattr(self._broker, 'wait_attributes'):
                try:
                    self._logger.info("Waiting for configuration response...")
                    attributes = self._broker.wait_attributes(response_matcher, timeout=30)
                    self._logger.info("Configuration applied successfully")
                    return True, None
                except Exception as e:
                    self._logger.warning(f"Could not verify configuration response: {e}")
                    self._logger.info("Configuration was sent, but response verification failed")
                    return True, None  # Still consider it success since we sent the config
            else:
                # Simple wait for state update
                self._logger.info("Waiting for configuration response (simplified)...")
                time.sleep(2)  # Give time for configuration to be processed

                # Check if we received a state update for this configuration
                if hasattr(self, 'received_attributes') and state_topic in self.received_attributes:
                    state_data = self.received_attributes[state_topic]
                    try:
                        if isinstance(state_data, str):
                            state_json = json.loads(state_data)
                        else:
                            state_json = state_data

                        if "res_info" in state_json and state_json["res_info"].get("res_id") == req_id:
                            code = state_json["res_info"].get("code", -1)
                            if code == 0:
                                self._logger.info("Configuration applied successfully")
                                return True, None
                            else:
                                detail_msg = state_json["res_info"].get("detail_msg", f"Error code: {code}")
                                self._logger.error(f"Configuration failed: {detail_msg}")
                                return False, detail_msg
                    except (json.JSONDecodeError, KeyError) as e:
                        self._logger.warning(f"Could not parse configuration response: {e}")

                self._logger.info("Configuration sent (response verification not available)")
                return True, None

        except Exception as e:
            self._logger.error(f"Error sending configuration: {e}")
            return False, str(e)


class DeploymentCLI:
    """Simple CLI for AI Model and Edge App deployment"""

    def __init__(self, http_host=None):
        self.logger = get_colored_logger(f"{self.__class__.__name__}")
        self.mqtt_broker = None
        self.http_server = None
        self.http_server_thread = None
        self.http_port = 8000
        self.http_host = http_host or get_local_ip()  # Use provided host or auto-detect
        self.http_server_running = False
        self.upload_dir = Path("./server_dir")
        self.upload_dir.mkdir(exist_ok=True)

        # Storage for deployed items
        self.deployed_ai_models = []
        self.deployed_modules = []

        # Load command history
        load_history()

        # Log the HTTP server address that will be used
        self.logger.info(f"HTTP server will use address: {self.http_host}")

    def set_webserver_log_suppression(self, suppress):
        """Helper function to control webserver log suppression"""
        try:
            from webserver import CustomHTTPRequestHandler
            CustomHTTPRequestHandler.set_log_suppression(suppress)
        except ImportError:
            pass  # Webserver not yet imported

    def start_http_server(self, port=None):
        """Start HTTP server"""
        if port is None:
            port = self.http_port
        else:
            self.http_port = port

        # Check if a server is already running on this port
        import socket
        def is_port_in_use(port):
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
                try:
                    s.bind(('', port))
                    return False  # Port is available
                except OSError:
                    return True   # Port is in use

        # Reset server state before checking
        self.http_server_running = False

        if is_port_in_use(self.http_port):
            self.logger.warning(f"Port {self.http_port} is already in use. HTTP server may already be running.")
            # Try to verify if our server is responsive
            try:
                import urllib.request
                urllib.request.urlopen(f"http://{self.http_host}:{self.http_port}", timeout=2)
                self.http_server_running = True
                self.logger.success(f"HTTP server is already running on {self.http_host}:{self.http_port}")
                return True
            except:
                # Port is in use but server is not responding
                # This might be a race condition or server still starting up
                self.logger.warning(f"Port {self.http_port} is in use by another process or server is still starting")

                # Try to proceed assuming server might become available
                # Wait a bit and try one more time
                time.sleep(2)
                try:
                    urllib.request.urlopen(f"http://{self.http_host}:{self.http_port}", timeout=3)
                    self.http_server_running = True
                    self.logger.success(f"HTTP server is now accessible on {self.http_host}:{self.http_port}")
                    return True
                except:
                    # Still not accessible, but assume it might work for deployment
                    self.logger.warning(f"HTTP server status unknown on port {self.http_port}. Proceeding optimistically.")
                    self.http_server_running = True  # Assume it's working
                    return True

        def run_http_server():
            try:
                from http.server import HTTPServer
                from webserver import create_handler

                # Create HTTP server instance that we can control
                def on_incoming_callback(dest_path):
                    pass  # Simple callback for file uploads

                handler = create_handler(on_incoming_callback, self.upload_dir)
                self.http_server = HTTPServer(("", self.http_port), handler)

                # Store server reference so we can shut it down later
                host, server_port = self.http_server.socket.getsockname()
                self.logger.debug(f"HTTP server created on {host}:{server_port}")

                # Start serving requests
                self.http_server_running = True
                self.http_server.serve_forever()

            except Exception as e:
                self.http_server_running = False
                self.logger.error(f"HTTP server error on port {self.http_port}: {e}")
            finally:
                # Ensure server is properly closed
                if hasattr(self, 'http_server') and self.http_server:
                    try:
                        self.http_server.server_close()
                    except:
                        pass

        # Stop existing server if running
        if hasattr(self, 'http_server_thread') and self.http_server_thread and self.http_server_thread.is_alive():
            self.logger.info("Stopping existing HTTP server...")
            try:
                if hasattr(self, 'http_server') and self.http_server:
                    self.http_server.shutdown()      # Gracefully stop server
                    self.http_server.server_close()  # Release socket
                self.http_server_thread.join(timeout=5)  # Wait for thread to finish
                if not self.http_server_thread.is_alive():
                    self.logger.success("HTTP server stopped successfully.")
                else:
                    self.logger.warning("Previous HTTP server did not stop cleanly")
                # Additional wait to ensure port is released
                time.sleep(1)
            except Exception as e:
                self.logger.warning(f"Error stopping previous HTTP server: {e}")
            finally:
                self.http_server = None
                self.http_server_thread = None
                self.http_server_running = False

        self.http_server_thread = threading.Thread(target=run_http_server, daemon=True)
        self.http_server_thread.start()

        # Give server time to start and check if it's running
        self.logger.info("Waiting to start http server...")
        time.sleep(2)  # Wait time to allow server startup

        # Verify server is actually running by checking port
        if is_port_in_use(self.http_port):
            self.http_server_running = True
            self.logger.info(f"HTTP server started successfully on {self.http_host}:{self.http_port}")
            self.logger.success(f"HTTP server started successfully on {self.http_host}:{self.http_port}")
            return True
        else:
            self.http_server_running = False
            self.logger.error(f"Failed to start HTTP server on {self.http_host}:{self.http_port}")
            return False

    def start_services(self, mqtt_host="localhost", mqtt_port=1883, http_port=8000, skip_device_init=False):
        """Start MQTT connection and HTTP server"""
        # Start MQTT connection
        self.mqtt_broker = SimpleMQTTBroker(mqtt_host, mqtt_port)
        if not self.mqtt_broker.connect():
            self.logger.error("Failed to connect to MQTT broker")
            return False

        # Wait for device initialization before proceeding (skip in command line mode for faster deployment)
        if not skip_device_init:
            self.logger.info("Waiting for device initialization...")
            if not self.mqtt_broker.wait_for_device_initialization(timeout=30):
                self.logger.warning("Device initialization timeout - CLI may not work properly")
                self.logger.warning("Continuing anyway, but commands may fail until device is initialized")
            else:
                self.logger.info("Device initialization completed - CLI ready for commands")
        else:
            self.logger.info("Skipping device initialization check for command line mode")

        # Try to start HTTP server
        self.http_port = http_port
        http_success = self.start_http_server(http_port)

        if not http_success:
            self.logger.warning("HTTP server failed to start. Use 'server retry' command to try again.")

        self.logger.info(f"Services started - MQTT: {mqtt_host}:{mqtt_port}, HTTP: {'OK' if http_success else 'FAILED'}")
        return True  # Return True even if HTTP fails, as MQTT is working

    def stop_services(self):
        """Stop all running services"""
        if self.mqtt_broker:
            self.logger.info("Stopping MQTT connection...")
            self.mqtt_broker.disconnect()
            self.mqtt_broker = None
            self.logger.success("MQTT connection stopped successfully")

        # Stop HTTP server thread if running
        if hasattr(self, 'http_server_thread') and self.http_server_thread and self.http_server_thread.is_alive():
            self.logger.info("Stopping HTTP server...")
            try:
                # First, try to gracefully shutdown the HTTP server
                if hasattr(self, 'http_server') and self.http_server:
                    self.logger.debug("Shutting down HTTP server...")
                    self.http_server.shutdown()
                    self.http_server.server_close()
                    self.logger.debug("HTTP server shutdown requested")

                # Wait for thread to finish with a reasonable timeout
                self.logger.debug("Waiting for HTTP server thread to finish...")
                self.http_server_thread.join(timeout=5)  # Increased timeout

                if not self.http_server_thread.is_alive():
                    self.logger.success(f"HTTP server stopped successfully on port {self.http_port}")
                else:
                    self.logger.warning("HTTP server thread did not stop within timeout - forcing termination")
                    # Force cleanup
                    try:
                        if hasattr(self, 'http_server') and self.http_server:
                            self.http_server.server_close()
                    except:
                        pass

                # Additional wait to ensure port is released
                time.sleep(1)

            except Exception as e:
                self.logger.warning(f"Error stopping HTTP server: {e}")
            finally:
                # Clean up references regardless of success/failure
                self.http_server_thread = None
                self.http_server = None
                self.http_server_running = False

        # Legacy process-based cleanup
        if hasattr(self, 'http_server_process') and self.http_server_process:
            self.logger.info(f"Stopping HTTP server (PID: {self.http_server_process.pid})...")
            try:
                self.http_server_process.terminate()
                # Give the server a moment to shut down gracefully
                self.http_server_process.wait(timeout=5)
                self.logger.success(f"HTTP server stopped successfully on port {self.http_port}")
            except Exception as e:
                self.logger.warning(f"Force killing HTTP server: {e}")
                self.http_server_process.kill()
                self.http_server_process.wait()
                self.logger.info("HTTP server force-stopped")
            finally:
                self.http_server_process = None

    def calculate_file_hash_wasm(self, file_path):
        """Calculate SHA256 hash of file and return as hex string (for WASM)"""
        hasher = hashlib.sha256()
        with open(file_path, 'rb') as f:
            for chunk in iter(lambda: f.read(4096), b""):
                hasher.update(chunk)
        return hasher.hexdigest()

    def calculate_file_hash_ai(self, file_path):
        """Calculate SHA256 hash of file and return as Base64 string (for AI model)"""
        import base64
        hasher = hashlib.sha256()
        with open(file_path, 'rb') as f:
            for chunk in iter(lambda: f.read(4096), b""):
                hasher.update(chunk)
        return base64.b64encode(hasher.digest()).decode('utf-8')

    def copy_file_to_upload_dir(self, source_path, filename=None):
        """Copy file to upload directory and return URL"""
        source = Path(source_path)
        if not source.exists():
            raise FileNotFoundError(f"File not found: {source_path}")

        dest_name = filename or source.name
        dest_path = self.upload_dir / dest_name

        # Log file copy operation
        self.logger.info(f"Copying file: {source.absolute()} -> {dest_path.absolute()}")

        shutil.copy2(source, dest_path)

        # Verify file was copied successfully
        if dest_path.exists():
            file_size = dest_path.stat().st_size
            self.logger.info(f"File copied successfully: {dest_path.absolute()} ({file_size} bytes)")
        else:
            self.logger.error(f"Failed to copy file to: {dest_path.absolute()}")
            raise RuntimeError(f"Failed to copy file to upload directory")

        # Generate URL using the configured HTTP server host and port
        url = f"http://{self.http_host}:{self.http_port}/{dest_name}"
        self.logger.info(f"Generated URL: {url}")
        return url, dest_path

    def deploy_ai_model(self, model_path, version=None):
        """Deploy AI model to RPi with deployment status monitoring and progress tracking"""
        try:
            model_path = Path(model_path)
            if not model_path.exists():
                self.logger.error(f"AI model file not found: {model_path}")
                return False

            # Use provided version or default
            if not version:
                version = "0311030000000123"  # Default version

            # Copy file to upload directory
            url, local_path = self.copy_file_to_upload_dir(model_path)

            # Additional check: Ensure HTTP server is running for file serving
            if not self.http_server_running:
                self.logger.warning("HTTP server is not running. Attempting to start for file serving...")
                if not self.start_http_server():
                    self.logger.warning("HTTP server startup failed, but continuing with deployment...")
                    # Continue anyway - URL was already generated

            # Calculate hash for AI model (Base64)
            file_hash = self.calculate_file_hash_ai(local_path)

            # Create AI model object
            ai_model = AIModel(
                name=model_path.stem,
                path=str(local_path),
                hash=file_hash,
                url=url,
                version=version,
                id=uuid4()
            )

            # Generate request ID for tracking (15 characters, alphanumeric)
            req_id = "".join(uuid4().hex[:15])

            # Enable verbose logging temporarily for progress tracking
            original_verbose = self.mqtt_broker.verbose_logging
            self.mqtt_broker.verbose_logging = True

            # Enable HTTP server logging during deployment
            self.set_webserver_log_suppression(False)

            try:
                # Publish via MQTT, matching the successful payload structure
                self.logger.info(f"Publishing AI model configuration (Request ID: {req_id})...")

                deploy_value = {
                        "targets": [
                            {
                                "chip": "sensor_chip",
                                "hash": file_hash,
                                "size": local_path.stat().st_size,
                                "version": version,  # Use specified version
                                "package_url": url
                            }
                        ],
                        "req_info": {
                            "req_id": req_id
                        }
                    }

                # Construct the payload to match the successful JSON file structure
                message = {
                    "configuration/$system/PRIVATE_deploy_ai_model": json.dumps(deploy_value)
                }

                # Log the message format for debugging
                self.logger.info(f"Publishing AI model configuration message:")
                self.logger.info(f"Message format: {json.dumps(message, indent=2)}")

                topic = "v1/devices/me/attributes"
                result = self.mqtt_broker.client.publish(topic, json.dumps(message))

                if result.rc != mqtt.MQTT_ERR_SUCCESS:
                    self.logger.error(f"Failed to publish AI model deployment: {result.rc}")
                    return False

                # Wait for deployment response with progress monitoring (extended timeout for large models)
                self.logger.info("Waiting for AI model deployment response...")
                self.logger.info("Progress will be displayed as the model downloads and installs...")
                self.logger.tip("Press Ctrl+C to cancel deployment (CLI will continue running)")

                try:
                    success, error = self.mqtt_broker.wait_for_ai_model_config_response(req_id, timeout=120)

                    if success:
                        self.deployed_ai_models.append(ai_model)
                        self.logger.success(f"AI model deployment completed successfully: {ai_model.name} (Version: {version})")
                        self.logger.info(f"  URL: {url}")
                        self.logger.info(f"  Hash: {file_hash}")
                        self.logger.info(f"  Size: {local_path.stat().st_size} bytes")
                        return True
                    else:
                        self.logger.error(f"AI model deployment failed: {error}")
                        return False

                except KeyboardInterrupt:
                    self.logger.warning("AI model deployment cancelled by user")
                    self.logger.info("Cleaning up deployment request...")
                    # Clean up the callback if it exists
                    self.mqtt_broker.ai_model_status_callbacks.pop(req_id, None)
                    # Suppress HTTP server logs after cancellation
                    self.set_webserver_log_suppression(True)
                    return False

            finally:
                # Restore original verbose logging setting
                self.mqtt_broker.verbose_logging = original_verbose
                # Suppress HTTP server logs after deployment completion
                self.set_webserver_log_suppression(True)

        except Exception as e:
            self.logger.error(f"Error deploying AI model: {e}")
            return False

    def deploy_wasm(self, wasm_path, module_name=None):
        """Deploy Edge App module to RPi with deployment status monitoring"""
        try:
            wasm_path = Path(wasm_path)
            if not wasm_path.exists():
                self.logger.error(f"Edge App file not found: {wasm_path}")
                return False

            # Generate module name if not provided
            if not module_name:
                module_name = wasm_path.stem

            # Copy file to upload directory
            url, local_path = self.copy_file_to_upload_dir(wasm_path)

            # Calculate hash for Edge App (hex)
            file_hash = self.calculate_file_hash_wasm(local_path)

            # Create module object
            module = Module(
                name=module_name,
                hash=file_hash,
                host=url,
                type="wasm",
                entry_point="main",
                id=uuid4(),
                instance=uuid4()
            )

            # Generate deployment ID for tracking
            deployment_id = str(uuid4())

            # Enable verbose logging temporarily for deployment tracking
            original_verbose = self.mqtt_broker.verbose_logging
            self.mqtt_broker.verbose_logging = True

            # Enable HTTP server logging during deployment
            self.set_webserver_log_suppression(False)

            try:
                # Create deployment message using OnWireSchema
                deployment = self.mqtt_broker.onwire_schema.to_deployment(deployment_id, [module])

                # Publish deployment
                self.logger.info(f"Publishing Edge App deployment (Deployment ID: {deployment_id})...")
                result = self.mqtt_broker.client.publish("v1/devices/me/attributes", json.dumps(deployment))

                if result.rc != mqtt.MQTT_ERR_SUCCESS:
                    self.logger.error(f"Failed to publish Edge App deployment: {result.rc}")
                    return False

                # Wait for deployment status
                self.logger.info("Waiting for Edge App deployment confirmation...")
                self.logger.info("Deployment status will be monitored...")
                self.logger.tip("Press Ctrl+C to cancel deployment (CLI will continue running)")

                try:
                    success, error = self.mqtt_broker.wait_for_deployment_status(deployment_id, timeout=60)

                    if success:
                        # Try to get the actual instance ID from device deployment status
                        time.sleep(1)  # Give time for deployment status to update
                        actual_instance_id = self._get_deployed_wasm_instance_by_hash(file_hash)

                        if actual_instance_id:
                            # Update the module with the actual instance ID
                            module.instance = actual_instance_id
                            self.logger.debug(f"Updated module instance ID to: {actual_instance_id}")

                        self.deployed_modules.append(module)
                        self.logger.success(f"Edge App deployment completed successfully: {module_name}")
                        self.logger.info(f"  URL: {url}")
                        self.logger.info(f"  Hash: {file_hash}")
                        self.logger.info(f"  Size: {local_path.stat().st_size} bytes")
                        if actual_instance_id:
                            self.logger.info(f"  Instance ID: {actual_instance_id}")
                        else:
                            self.logger.warning("  Instance ID: Not found (deployment status not available)")
                        return True
                    else:
                        self.logger.error(f"Edge App deployment failed: {error}")
                        return False

                except KeyboardInterrupt:
                    self.logger.warning("Edge App deployment cancelled by user")
                    self.logger.info("Cleaning up deployment request...")
                    # Clean up the callback if it exists
                    self.mqtt_broker.deployment_status_callbacks.pop(deployment_id, None)
                    # Suppress HTTP server logs after cancellation
                    self.set_webserver_log_suppression(True)
                    return False

            finally:
                # Restore original verbose logging setting
                self.mqtt_broker.verbose_logging = original_verbose
                # Suppress HTTP server logs after deployment completion
                self.set_webserver_log_suppression(True)

        except Exception as e:
            self.logger.error(f"Error deploying WASM: {e}")
            return False

    def undeploy_wasm(self):
        """Undeploy all Edge App modules from RPi"""
        try:
            # Generate deployment ID for tracking
            deployment_id = str(uuid4())

            # Enable verbose logging temporarily for deployment tracking
            original_verbose = self.mqtt_broker.verbose_logging
            self.mqtt_broker.verbose_logging = True

            try:
                # Create undeployment message using OnWireSchema (empty modules list for undeployment)
                deployment = self.mqtt_broker.onwire_schema.to_deployment(deployment_id, [])

                # Publish undeployment
                self.logger.info(f"Publishing Edge App undeployment (Deployment ID: {deployment_id})...")
                result = self.mqtt_broker.client.publish("v1/devices/me/attributes", json.dumps(deployment))

                if result.rc != mqtt.MQTT_ERR_SUCCESS:
                    self.logger.error(f"Failed to publish Edge App undeployment: {result.rc}")
                    return False

                # Wait for deployment status
                self.logger.info("Waiting for Edge App undeployment confirmation...")
                self.logger.info("Undeployment status will be monitored...")
                self.logger.tip("Press Ctrl+C to cancel waiting (CLI will continue running)")

                try:
                    success, error = self.mqtt_broker.wait_for_deployment_status(deployment_id, timeout=60)

                    if success:
                        # Clear deployed modules from local tracking
                        instance_count = len(self.deployed_modules)
                        self.deployed_modules.clear()
                        self.logger.success("Edge App undeployment completed successfully")
                        self.logger.info(f"Cleared {instance_count} instances from CLI tracking")
                        return True
                    else:
                        self.logger.error(f"Edge App undeployment failed: {error}")
                        return False

                except KeyboardInterrupt:
                    self.logger.warning("Edge App undeployment waiting cancelled by user")
                    self.logger.info("Undeployment may still be in progress on device...")
                    # Clean up the callback if it exists
                    self.mqtt_broker.deployment_status_callbacks.pop(deployment_id, None)
                    return False

            finally:
                # Restore original verbose logging setting
                self.mqtt_broker.verbose_logging = original_verbose

        except Exception as e:
            self.logger.error(f"Error undeploying WASM: {e}")
            return False

    def list_deployed(self):
        """List deployed items with enhanced device detection"""
        self.logger.info("=== Deployed Items ===")

        # AI Models section
        self.logger.info("AI Models")
        ai_models_shown = False

        # Show CLI-recorded AI models (with warning if not detected from device)
        if self.deployed_ai_models:
            for model in self.deployed_ai_models:
                # Check if this model is detected from device
                detected_from_device = self._is_ai_model_detected_from_device(model)
                if detected_from_device:
                    self.logger.success(f"  - {model.name} (v{model.version}) (detected from device)")
                else:
                    self.logger.warning(f"  - {model.name} (v{model.version}) (managed by CLI)")
                ai_models_shown = True

        # Show AI models detected from device but not recorded by CLI
        try:
            if self.mqtt_broker and self.mqtt_broker.received_attributes:
                detected_models = self._get_ai_models_from_device()
                self.logger.debug(f"Detected AI models: {detected_models}")
                for model_info in detected_models:
                    if isinstance(model_info, dict):
                        # Check if this model is already shown
                        model_already_shown = any(
                            self._is_ai_model_hash_match(model, model_info.get('hash', ''))
                            for model in self.deployed_ai_models
                        )
                        if not model_already_shown:
                            model_name = model_info.get("name", f"hash_{model_info.get('hash', 'unknown')[:8]}")
                            model_version = model_info.get("version", "unknown")
                            self.logger.success(f"  - {model_name} (v{model_version}) (detected from device)")
                            ai_models_shown = True
                    else:
                        self.logger.debug(f"Skipping invalid model info: {model_info} (type: {type(model_info)})")
        except Exception as e:
            self.logger.error(f"Error processing detected AI models: {e}")
            self.logger.debug(f"Exception details: {e.__class__.__name__}: {e}")

        if not ai_models_shown:
            self.logger.info("  None")

        # Edge App Modules section
        self.logger.info("Edge App Modules")
        wasm_modules_shown = False

        # Get all detected instances from device first
        all_detected_instances = {}  # instance_id -> instance_info
        try:
            detected_instances = self._get_wasm_instances_from_device()
            self.logger.debug(f"Detected instances: {detected_instances}")
            for instance_info in detected_instances:
                if isinstance(instance_info, dict):
                    instance_id = instance_info.get('instance_id')
                    if instance_id:
                        all_detected_instances[instance_id] = instance_info
        except Exception as e:
            self.logger.error(f"Error getting detected Edge App instances: {e}")
            self.logger.debug(f"Exception details: {e.__class__.__name__}: {e}")

        # Get CLI-recorded instance IDs (those that were deployed via CLI)
        cli_recorded_instances = set()
        if self.deployed_modules:
            for module in self.deployed_modules:
                # Use the actual instance ID that was recorded during deployment
                if hasattr(module, 'instance') and module.instance:
                    cli_recorded_instances.add(str(module.instance))

        # Show all detected instances (both CLI-recorded and device-only)
        for instance_id, instance_info in all_detected_instances.items():
            if instance_id in cli_recorded_instances:
                # This was deployed via CLI and is detected from device
                self.logger.success(f"  - {instance_id} (detected from device)")
            else:
                # This is detected from device but not in CLI records
                self.logger.success(f"  - {instance_id} (detected from device)")
            wasm_modules_shown = True

        # Show CLI-recorded instances that are NOT detected from device
        for instance_id in cli_recorded_instances:
            if instance_id not in all_detected_instances:
                self.logger.warning(f"  - {instance_id} (managed by CLI)")
                wasm_modules_shown = True

        if not wasm_modules_shown:
            self.logger.info("  None")

    def _is_ai_model_hash_match(self, model, device_hash):
        """Check if AI model hash matches device hash"""
        return model.hash == device_hash

    def _is_ai_model_detected_from_device(self, model):
        """Check if AI model is detected from device MQTT messages"""
        if not self.mqtt_broker or not self.mqtt_broker.received_attributes:
            self.logger.debug("No MQTT broker or received attributes")
            return False

        # Debug: Show all available keys
        self.logger.debug(f"Available MQTT attribute keys: {list(self.mqtt_broker.received_attributes.keys())}")

        for key, value in self.mqtt_broker.received_attributes.items():
            if "PRIVATE_deploy_ai_model" in key and "state/" in key:
                self.logger.debug(f"Found AI model key: {key}")
                self.logger.debug(f"Value type: {type(value)}, Value: {value}")
                try:
                    if isinstance(value, str):
                        state_data = json.loads(value)
                    else:
                        state_data = value

                    if "targets" in state_data:
                        for target in state_data["targets"]:
                            if isinstance(target, dict):
                                progress = target.get("progress", 0)
                                process_state = target.get("process_state", "unknown")
                                target_hash = target.get("hash", "")

                                # Debug logging
                                self.logger.debug(f"Checking AI model: progress={progress}, state={process_state}, hash_match={target_hash == model.hash}")
                                self.logger.debug(f"Target hash: {target_hash}, Model hash: {model.hash}")

                                if progress >= 100 and process_state == "done" and target_hash == model.hash:
                                    return True
                except (json.JSONDecodeError, TypeError) as e:
                    self.logger.debug(f"Error parsing AI model state: {e}")
                    pass
        return False

    def _get_ai_models_from_device(self):
        """Get AI models detected from device MQTT messages"""
        models = []
        if not self.mqtt_broker or not self.mqtt_broker.received_attributes:
            return models

        for key, value in self.mqtt_broker.received_attributes.items():
            if "PRIVATE_deploy_ai_model" in key and "state/" in key:
                try:
                    if isinstance(value, str):
                        state_data = json.loads(value)
                    else:
                        state_data = value

                    if "targets" in state_data:
                        for target in state_data["targets"]:
                            if isinstance(target, dict):
                                progress = target.get("progress", 0)
                                process_state = target.get("process_state", "unknown")
                                if progress >= 100 and process_state == "done":
                                    hash_val = target.get("hash", "unknown")
                                    models.append({
                                        "hash": hash_val,
                                        "size": target.get("size", 0),
                                        "name": f"model_{hash_val[:8]}" if hash_val != "unknown" else "unknown_model",
                                        "version": target.get("version", "unknown")
                                    })
                except (json.JSONDecodeError, TypeError):
                    pass
        return models

    def _is_wasm_module_detected_from_device(self, module):
        """Check if Edge App module is detected from device and return instance ID"""
        if not self.mqtt_broker or not self.mqtt_broker.received_attributes:
            return None

        deployment_status = self.mqtt_broker.received_attributes.get("deploymentStatus")
        if not deployment_status:
            return None

        try:
            if isinstance(deployment_status, str):
                deployment_data = json.loads(deployment_status)
            else:
                deployment_data = deployment_status

            # Look for instances in deployment status
            instances = deployment_data.get("instances", {})
            modules = deployment_data.get("modules", {})

            for instance_id, instance_info in instances.items():
                if instance_info.get("status") == "ok":
                    module_id = instance_info.get("moduleId")
                    if module_id and module_id in modules:
                        module_info = modules[module_id]
                        # Check if hash matches
                        if module_info.get("hash") == module.hash:
                            return instance_id

        except (json.JSONDecodeError, KeyError):
            pass

        return None

    def _get_deployed_wasm_instance_by_hash(self, target_hash):
        """Get the deployed Edge App instance ID by matching hash"""
        if not self.mqtt_broker or not self.mqtt_broker.received_attributes:
            return None

        # First, try to find instance ID from the deployment message itself
        deployment_message = self.mqtt_broker.received_attributes.get("deployment")
        if deployment_message:
            try:
                if isinstance(deployment_message, str):
                    deployment_data = json.loads(deployment_message)
                else:
                    deployment_data = deployment_message

                # Look for modules in deployment message
                modules = deployment_data.get("modules", {})
                instance_specs = deployment_data.get("instanceSpecs", {})

                for module_id, module_info in modules.items():
                    if module_info.get("hash") == target_hash:
                        # Find the instance ID that uses this module
                        for instance_id, instance_spec in instance_specs.items():
                            if instance_spec.get("moduleId") == module_id:
                                self.logger.debug(f"Found instance ID from deployment message: {instance_id}")
                                return instance_id

            except (json.JSONDecodeError, KeyError) as e:
                self.logger.debug(f"Error parsing deployment message: {e}")

        # Fallback to deploymentStatus
        deployment_status = self.mqtt_broker.received_attributes.get("deploymentStatus")
        if not deployment_status:
            return None

        try:
            if isinstance(deployment_status, str):
                deployment_data = json.loads(deployment_status)
            else:
                deployment_data = deployment_status

            # Look for instances in deployment status
            instances = deployment_data.get("instances", {})
            modules = deployment_data.get("modules", {})

            for instance_id, instance_info in instances.items():
                if instance_info.get("status") == "ok":
                    module_id = instance_info.get("moduleId")
                    if module_id and module_id in modules:
                        module_info = modules[module_id]
                        # Check if hash matches
                        if module_info.get("hash") == target_hash:
                            self.logger.debug(f"Found instance ID from deployment status: {instance_id}")
                            return instance_id

        except (json.JSONDecodeError, KeyError) as e:
            self.logger.debug(f"Error parsing deployment status: {e}")

        return None

    def _get_wasm_instances_from_device(self):
        """Get Edge App instances detected from device deployment status"""
        instances = []
        if not self.mqtt_broker or not self.mqtt_broker.received_attributes:
            self.logger.debug("No MQTT broker or received attributes")
            return instances

        deployment_status = self.mqtt_broker.received_attributes.get("deploymentStatus")
        if not deployment_status:
            self.logger.debug("No deploymentStatus found")
            return instances

        self.logger.debug(f"deploymentStatus type: {type(deployment_status)}, value: {deployment_status}")

        try:
            if isinstance(deployment_status, str):
                deployment_data = json.loads(deployment_status)
            else:
                deployment_data = deployment_status

            # Look for instances in deployment status
            instance_specs = deployment_data.get("instances", {})
            modules = deployment_data.get("modules", {})

            self.logger.debug(f"Found {len(instance_specs)} instances in deployment status")

            for instance_id, instance_info in instance_specs.items():
                self.logger.debug(f"Processing instance {instance_id}: {instance_info}")
                if instance_info.get("status") == "ok":
                    module_id = instance_info.get("moduleId")
                    module_name = instance_info.get("name", "unknown")
                    instances.append({
                        "instance_id": instance_id,
                        "module_name": module_name,
                        "module_id": module_id
                    })
                    self.logger.debug(f"Added instance: {instance_id}, name: {module_name}")

        except (json.JSONDecodeError, KeyError) as e:
            self.logger.debug(f"Error parsing deployment status: {e}")
            pass

        self.logger.debug(f"Returning {len(instances)} instances")
        return instances

    def show_server_status(self):
        """Show server status"""
        self.logger.info("=== Server Status ===")

        # MQTT status
        if self.mqtt_broker and self.mqtt_broker.connected:
            self.logger.info(f"MQTT Broker: Connected ({self.mqtt_broker.host}:{self.mqtt_broker.port})")
        else:
            self.logger.info("MQTT Broker: Disconnected")

        # HTTP server status - check actual port usage
        import socket
        def is_port_in_use(port):
            with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
                try:
                    s.bind(('', port))
                    return False
                except OSError:
                    return True

        port_in_use = is_port_in_use(self.http_port)

        if port_in_use:
            self.logger.info(f"HTTP Server: Running ({self.http_host}:{self.http_port})")
            self.logger.info(f"Upload Directory: {self.upload_dir.absolute()}")

            # Show files in upload directory
            if self.upload_dir.exists():
                files = list(self.upload_dir.glob("*"))
                if files:
                    self.logger.info("Available files:")
                    for file in files:
                        if file.is_file():
                            size = file.stat().st_size
                            self.logger.info(f"  - {file.name} ({size} bytes)")
                else:
                    self.logger.warning("No files in server directory")

            self.http_server_running = True  # Update status
        else:
            self.logger.info(f"HTTP Server: Not running ({self.http_host}:{self.http_port} available)")
            self.logger.info("Use 'server retry [port]' to start the HTTP server")
            self.http_server_running = False  # Update status

    def show_command_history(self):
        """Show command history"""
        self.logger.info("=== Command History ===")
        history_length = readline.get_current_history_length()
        if history_length > 0:
            # Show last 10 commands
            start = max(1, history_length - 9)
            for i in range(start, history_length + 1):
                command = readline.get_history_item(i)
                if command:
                    self.logger.info(f"  {i}: {command}")
        else:
            self.logger.info("No command history available")

    def get_deployed_wasm_instance_id(self):
        """Get the deployed Edge App instance ID from deployment status"""
        if not self.mqtt_broker or not hasattr(self.mqtt_broker, 'received_attributes'):
            return None

        deployment_status = self.mqtt_broker.received_attributes.get("deploymentStatus")
        if not deployment_status:
            return None

        try:
            if isinstance(deployment_status, str):
                deployment_data = json.loads(deployment_status)
            else:
                deployment_data = deployment_status

            # Look for instances in deployment status
            instances = deployment_data.get("instances", {})
            for instance_id, instance_info in instances.items():
                if instance_info.get("status") == "ok":
                    self.logger.debug(f"Found deployed Edge App instance: {instance_id}")
                    return instance_id

        except (json.JSONDecodeError, KeyError) as e:
            self.logger.debug(f"Could not parse deployment status: {e}")

        return None

    def _check_device_initialization(self):
        """Check if device is initialized and wait if necessary"""
        if not self.mqtt_broker:
            self.logger.error("MQTT broker not connected")
            return False

        if not self.mqtt_broker.device_initialized:
            self.logger.debug("Device not yet initialized, waiting...")
            if not self.mqtt_broker.wait_for_device_initialization(timeout=10):
                self.logger.error("Device initialization failed - command may not work properly")
                return False

        return True

    def send_configuration(self, config_file_path, instance="auto", ps=None, topic="edge_app"):
        """Send configuration from JSON file to Edge App via MQTT with process_state options"""
        try:
            config_path = Path(config_file_path)

            # Try different locations for configuration file
            if not config_path.exists():
                # Try in current directory
                config_path = Path(".") / config_file_path
                if not config_path.exists():
                    # Try with .json extension
                    if not config_file_path.endswith(".json"):
                        config_path = Path(config_file_path + ".json")
                        if not config_path.exists():
                            config_path = Path(".") / (config_file_path + ".json")

            if not config_path.exists():
                self.logger.error(f"Configuration file not found: {config_file_path}")
                self.logger.info("Searched in:")
                self.logger.info(f"  - {Path(config_file_path).absolute()}")
                self.logger.info(f"  - {(Path('.') / config_file_path).absolute()}")
                if not config_file_path.endswith(".json"):
                    self.logger.info(f"  - {Path(config_file_path + '.json').absolute()}")
                    self.logger.info(f"  - {(Path('.') / (config_file_path + '.json')).absolute()}")
                return False

            # Load configuration file
            self.logger.info(f"Loading configuration from: {config_path.absolute()}")
            with open(config_path, 'r', encoding='utf-8') as f:
                config_data = json.load(f)

            # Auto-generate unique req_id to avoid "req_id is null" errors
            if "edge_app" in config_data:
                if "req_info" not in config_data["edge_app"]:
                    config_data["edge_app"]["req_info"] = {}

                # Generate a unique request ID with timestamp (no underscores)
                ts = str(int(time.time()))
                rand = str(uuid4()).replace('-', '')[:8]
                new_req_id = f"cli{ts}{rand}"
                old_req_id = config_data["edge_app"]["req_info"].get("req_id", "not set")
                config_data["edge_app"]["req_info"]["req_id"] = new_req_id
                self.logger.info(f"Updated req_id: {old_req_id} -> {new_req_id}")
            else:
                # If no edge_app wrapper, apply req_id directly to the config
                # This handles cases where config structure is already flattened
                if "req_info" not in config_data:
                    config_data["req_info"] = {}

                # Generate a unique request ID with timestamp
                ts = str(int(time.time()))
                rand = str(uuid4()).replace('-', '')[:8]
                new_req_id = f"cli{ts}{rand}"
                old_req_id = config_data["req_info"].get("req_id", "not set")
                config_data["req_info"]["req_id"] = new_req_id
                self.logger.info(f"Updated req_id: {old_req_id} -> {new_req_id}")

            # Apply process_state option if specified
            if ps is not None:
                if "edge_app" in config_data and "common_settings" in config_data["edge_app"]:
                    old_ps = config_data["edge_app"]["common_settings"].get("process_state", "not set")
                    config_data["edge_app"]["common_settings"]["process_state"] = ps
                    self.logger.info(f"Updated process_state: {old_ps} -> {ps}")
                elif "common_settings" in config_data:
                    # Handle flattened config structure
                    old_ps = config_data["common_settings"].get("process_state", "not set")
                    config_data["common_settings"]["process_state"] = ps
                    self.logger.info(f"Updated process_state: {old_ps} -> {ps}")
                else:
                    self.logger.warning("process_state option specified but common_settings not found in configuration")

            # Resolve instance ID
            target_instance = instance
            if instance == "auto":
                # Try to auto-detect deployed Edge App instance
                detected_instance = self.get_deployed_wasm_instance_id()
                if detected_instance:
                    target_instance = detected_instance
                    self.logger.info(f"Auto-detected Edge App instance: {target_instance}")
                elif instance == "auto":
                    self.logger.warning("Could not auto-detect Edge App instance, using $system")
                    target_instance = "$system"
                else:
                    self.logger.warning("No deployed Edge App instance found, using $system (may not work for Edge App configuration)")
                    target_instance = "$system"

            # Send configuration via MQTT
            self.logger.info(f"Sending configuration to instance '{target_instance}'...")

            # Log the final configuration structure for debugging
            self.logger.debug("Final configuration structure:")
            self.logger.debug(json.dumps(config_data, indent=2, ensure_ascii=False))

            # Enable verbose logging temporarily to see the response
            original_verbose = self.mqtt_broker.verbose_logging
            self.mqtt_broker.broker_log_on()

            try:
                success, error = self.mqtt_broker.send_configuration(config_data, target_instance, topic)

                if success:
                    self.logger.success(f"Configuration sent successfully to instance '{target_instance}'")

                    # Wait a bit and check state messages for validation
                    self.logger.info("Checking state messages for confirmation...")
                    time.sleep(3)

                    # Look for state messages related to our configuration
                    if hasattr(self.mqtt_broker, 'received_attributes'):
                        edge_app_states = {}
                        for key, value in self.mqtt_broker.received_attributes.items():
                            if key.startswith("state/") and "edge_app" in key:
                                edge_app_states[key] = value

                        if edge_app_states:
                            self.logger.info("Edge App state information found:")
                            for state_key, state_value in edge_app_states.items():
                                try:
                                    if isinstance(state_value, str):
                                        state_json = json.loads(state_value)
                                    else:
                                        state_json = state_value

                                    # Display relevant state information
                                    if "res_info" in state_json:
                                        res_info = state_json["res_info"]
                                        code = res_info.get("code", -1)
                                        detail_msg = res_info.get("detail_msg", "")

                                        if code == 0:
                                            self.logger.success(f"State validation: {state_key} - SUCCESS")
                                        else:
                                            self.logger.warning(f"State validation: {state_key} - Code: {code}, Detail: {detail_msg}")

                                    # Show process_state if available
                                    if "process_state" in state_json:
                                        current_ps = state_json["process_state"]
                                        self.logger.info(f"Current process_state: {current_ps}")

                                except (json.JSONDecodeError, KeyError) as e:
                                    self.logger.debug(f"Could not parse state for {state_key}: {e}")
                        else:
                            self.logger.info("No Edge App state messages found yet")

                    return True
                else:
                    self.logger.error(f"Configuration sending failed: {error}")
                    return False

            finally:
                # Restore original verbose setting
                self.mqtt_broker.verbose_logging = original_verbose

        except json.JSONDecodeError as e:
            self.logger.error(f"Invalid JSON in configuration file: {e}")
            return False
        except Exception as e:
            self.logger.error(f"Error sending configuration: {e}")
            return False

    def show_deployed_instances(self):
        """Show deployed Edge App instances from deployment status"""
        self.logger.info("=== Deployed Edge App Instances ===")

        if not self.mqtt_broker or not hasattr(self.mqtt_broker, 'received_attributes'):
            self.logger.error("MQTT broker not connected")
            return

        deployment_status = self.mqtt_broker.received_attributes.get("deploymentStatus")
        if not deployment_status:
            self.logger.info("No deployment status available")
            return

        try:
            if isinstance(deployment_status, str):
                deployment_data = json.loads(deployment_status)
            else:
                deployment_data = deployment_status

            # Show deployment ID
            deployment_id = deployment_data.get("deploymentId", "unknown")
            reconcile_status = deployment_data.get("reconcileStatus", "unknown")
            self.logger.info(f"Deployment ID: {deployment_id}")
            self.logger.info(f"Reconcile Status: {reconcile_status}")

            # Show instances
            instances = deployment_data.get("instances", {})
            if instances:
                self.logger.info("Instances:")
                for instance_id, instance_info in instances.items():
                    status = instance_info.get("status", "unknown")
                    module_id = instance_info.get("moduleId", "unknown")

                    status_icon = "✓" if status == "ok" else "✗"
                    self.logger.info(f"  {status_icon} {instance_id}")
                    self.logger.info(f"    Status: {status}")
                    self.logger.info(f"    Module ID: {module_id}")

                # Show which instance would be auto-detected
                auto_instance = self.get_deployed_wasm_instance_id()
                if auto_instance:
                    self.logger.info(f"\nAuto-detected instance: {auto_instance}")
                else:
                    self.logger.warning("No active instance found for auto-detection")
            else:
                self.logger.info("No instances found")

            # Show modules
            modules = deployment_data.get("modules", {})
            if modules:
                self.logger.info("\nModules:")
                for module_id, module_info in modules.items():
                    status = module_info.get("status", "unknown")
                    status_icon = "✓" if status == "ok" else "✗"
                    self.logger.info(f"  {status_icon} {module_id} ({status})")

        except (json.JSONDecodeError, KeyError) as e:
            self.logger.error(f"Could not parse deployment status: {e}")

    def show_mqtt_status(self):
        """Show MQTT attributes and status information"""
        self.logger.info("=== MQTT Status ===")

        if not self.mqtt_broker or not self.mqtt_broker.connected:
            self.logger.error("MQTT broker not connected")
            return

        # Show connection status
        self.logger.info(f"MQTT Connection: Connected to {self.mqtt_broker.host}:{self.mqtt_broker.port}")

        # Show received attributes
        if self.mqtt_broker.received_attributes:
            self.logger.info("=== Received MQTT Attributes ===")

            # Filter and display state/ keys
            state_keys = {k: v for k, v in self.mqtt_broker.received_attributes.items() if k.startswith("state/")}
            if state_keys:
                self.logger.info("State Keys:")
                for key, value in state_keys.items():
                    try:
                        if isinstance(value, str) and value.startswith("{"):
                            formatted_value = json.loads(value)
                            self.logger.info(f"  {key}: {json.dumps(formatted_value, indent=2, ensure_ascii=False)}")
                        else:
                            self.logger.info(f"  {key}: {value}")
                    except json.JSONDecodeError:
                        self.logger.info(f"  {key}: {value}")

            # Show deployment status if available
            deployment_status = self.mqtt_broker.received_attributes.get("deploymentStatus")
            if deployment_status:
                self.logger.info("=== Deployment Status ===")
                self.logger.info(f"  {json.dumps(deployment_status, indent=2, ensure_ascii=False)}")

        else:
            self.logger.info("No MQTT attributes received yet")

        # Show active callbacks
        active_callbacks = len(self.mqtt_broker.deployment_status_callbacks) + len(self.mqtt_broker.ai_model_status_callbacks)
        if active_callbacks > 0:
            self.logger.info(f"Active deployment monitors: {active_callbacks}")
        else:
            self.logger.info("No active deployment monitors")

    def run_cli(self):
        """Run interactive CLI"""
        self.logger.info("=== AI Model & Edge App Deployment CLI ===")
        self.logger.info("Note: This CLI waits for device initialization before accepting deployment commands")
        self.logger.info("Commands:")
        self.logger.info("  deploy ai <path> [version]        - Deploy AI model (version is optional)")
        self.logger.info("  deploy edge_app <path> [module_name]  - Deploy Edge App module (module_name is optional)")
        self.logger.info("  undeploy                          - Undeploy all Edge App modules")
        self.logger.info("  send conf [config_file] [instance] [ps=<value>] - Send configuration via MQTT")
        self.logger.info("  instances                         - Show deployed Edge App instances")
        self.logger.info("  list                              - List deployed items")
        self.logger.info("  log on/off                        - Enable or disable verbose MQTT logging")
        self.logger.info("  messages                          - Show recent 3 MQTT messages")
        self.logger.info("  status                            - Show MQTT attributes status")
        self.logger.info("  server retry [port]               - Retry starting HTTP server")
        self.logger.info("  server status                     - Show server status")
        self.logger.info("  history                           - Show command history")
        self.logger.info("  help                              - Show this help")
        self.logger.info("  exit                              - Exit CLI")
        self.logger.info("")
        self.logger.info("Configuration examples:")
        self.logger.info("  send conf                                       - Use configuration.json, auto-detect instance")
        self.logger.info("  send conf ps=1                                  - Use configuration.json, ps=1, auto-detect instance")
        self.logger.info("  send conf configuration.json                    - Specify config file, auto-detect instance")
        self.logger.info("  send conf configuration.json ps=1               - Specify config file + process_state=1")
        self.logger.info("  send conf configuration.json <instance_id>      - Send to specific instance")
        self.logger.info("")
        self.logger.info("Note: Press Ctrl+C during deployment to cancel operation (CLI will continue)")

        while True:
            try:
                command = input("> ").strip()
                if not command:
                    continue

                parts = command.split()

                if parts[0] == "exit":
                    break
                elif parts[0] == "help":
                    self.logger.info("Note: This CLI waits for device initialization before accepting deployment commands")
                    self.logger.info("Commands:")
                    self.logger.info("  deploy ai <path> [version]        - Deploy AI model (version is optional)")
                    self.logger.info("  deploy edge_app <path> [module_name]  - Deploy Edge App module (module_name is optional)")
                    self.logger.info("  undeploy                          - Undeploy all Edge App modules")
                    self.logger.info("  send conf [config_file] [instance] [ps=<value>] - Send configuration via MQTT")
                    self.logger.info("  instances                         - Show deployed Edge App instances")
                    self.logger.info("  list                              - List deployed items")
                    self.logger.info("  log on/off                        - Enable or disable verbose MQTT logging")
                    self.logger.info("  messages                          - Show recent 3 MQTT messages")
                    self.logger.info("  status                            - Show MQTT attributes status")
                    self.logger.info("  server retry [port]               - Retry starting HTTP server")
                    self.logger.info("  server status                     - Show server status")
                    self.logger.info("  history                           - Show command history")
                    self.logger.info("  help                              - Show this help")
                    self.logger.info("  exit                              - Exit CLI")
                    self.logger.info("")
                    self.logger.info("Configuration examples:")
                    self.logger.info("  send conf                                       - Use configuration.json, auto-detect instance")
                    self.logger.info("  send conf ps=1                                  - Use configuration.json, ps=1, auto-detect instance")
                    self.logger.info("  send conf configuration.json                    - Specify config file, auto-detect instance")
                    self.logger.info("  send conf configuration.json ps=1               - Specify config file + process_state=1")
                    self.logger.info("  send conf configuration.json <instance_id>      - Send to specific instance")
                    self.logger.info("")
                    self.logger.info("Note: Press Ctrl+C during deployment to cancel operation (CLI will continue)")
                    self.logger.info("Navigation: Use ↑/↓ arrow keys to browse command history")
                elif parts[0] == "history":
                    self.show_command_history()
                elif parts[0] == "instances":
                    self.show_deployed_instances()
                elif parts[0] == "list":
                    self.list_deployed()
                elif parts[0] == "send" and len(parts) >= 2 and parts[1] == "conf":
                    # Check device initialization before sending configuration
                    if not self._check_device_initialization():
                        self.logger.error("Cannot execute send conf command - device not initialized")
                        continue

                    # Parse 'send conf' command
                    config_file = "configuration.json"  # Default configuration file
                    instance = "auto"  # Default to auto-detection
                    ps = None

                    # Parse arguments starting from parts[2]
                    for arg in parts[2:]:
                        if arg.startswith("ps="):
                            try:
                                ps = int(arg.split("=")[1])
                            except (ValueError, IndexError):
                                self.logger.error(f"Invalid ps value: {arg}")
                                continue
                        elif arg.endswith(".json"):
                            # This is likely a configuration file
                            config_file = arg
                        else:
                            # Assume it's an instance name
                            instance = arg

                    # Send configuration
                    self.send_configuration(config_file, instance, ps)
                elif parts[0] == "log" and len(parts) >= 2:
                    if parts[1] == "on":
                        if self.mqtt_broker:
                            self.mqtt_broker.broker_log_on()
                            self.logger.info("Verbose MQTT logging enabled")
                        else:
                            self.logger.error("MQTT broker not connected")
                    elif parts[1] == "off":
                        if self.mqtt_broker:
                            self.mqtt_broker.broker_log_off()
                            self.logger.info("Verbose MQTT logging disabled")
                        else:
                            self.logger.error("MQTT broker not connected")
                    else:
                        self.logger.error("Invalid log command. Use 'log on' or 'log off'")
                elif parts[0] == "messages":
                    if self.mqtt_broker:
                        self.mqtt_broker.show_recent_messages()
                    else:
                        self.logger.error("MQTT broker not connected")
                elif parts[0] == "status":
                    if self.mqtt_broker:
                        self.show_mqtt_status()
                    else:
                        self.logger.error("MQTT broker not connected")
                elif parts[0] == "server" and len(parts) >= 2:
                    if parts[1] == "retry":
                        retry_port = int(parts[2]) if len(parts) > 2 else self.http_port
                        self.logger.info(f"Retrying HTTP server on port {retry_port}...")
                        success = self.start_http_server(retry_port)
                        if success:
                            self.logger.info("HTTP server started successfully!")
                            self.logger.success("HTTP server started successfully!")
                        else:
                            self.logger.error("HTTP server retry failed. Try a different port or check if port is in use.")
                    elif parts[1] == "status":
                        self.show_server_status()
                    else:
                        self.logger.error("Invalid server command. Use 'server retry [port]' or 'server status'")
                elif parts[0] == "deploy" and len(parts) >= 3:
                    # Check device initialization before deployment
                    if not self._check_device_initialization():
                        self.logger.error("Cannot execute deploy command - device not initialized")
                        continue

                    # Check if HTTP server is running before deployment
                    if not self.http_server_running:
                        self.logger.warning("HTTP server is not running. Files may not be accessible for deployment.")
                        self.logger.info("Use 'server retry' to start the HTTP server.")
                        self.logger.tip("Use 'server retry' to start the HTTP server.")

                    try:
                        if parts[1] == "ai":
                            model_path = parts[2]
                            version = parts[3] if len(parts) > 3 else None
                            self.deploy_ai_model(model_path, version)
                        elif parts[1] == "edge_app":
                            wasm_path = parts[2]
                            module_name = parts[3] if len(parts) > 3 else None
                            self.deploy_wasm(wasm_path, module_name)
                        else:
                            self.logger.error("Invalid deploy command. Use 'deploy ai' or 'deploy edge_app'")
                    except KeyboardInterrupt:
                        self.logger.warning("Deployment operation cancelled")
                elif parts[0] == "undeploy":
                    # Check device initialization before undeployment
                    if not self._check_device_initialization():
                        self.logger.error("Cannot execute undeploy command - device not initialized")
                        continue

                    try:
                        self.undeploy_wasm()
                    except KeyboardInterrupt:
                        self.logger.warning("Undeployment operation cancelled")
                else:
                    self.logger.error("Unknown command. Type 'help' for available commands.")

            except KeyboardInterrupt:
                # Only exit CLI if not in a deployment operation
                self.logger.info("Received interrupt signal, exiting...")
                break
            except Exception as e:
                self.logger.error(f"Command error: {e}")

        # Save command history before exiting
        self.logger.info("Saving command history...")
        save_history()

        # Stop all services before exiting
        self.logger.info("Stopping services...")
        self.stop_services()

        self.logger.info("CLI shutdown complete")


def main():
    """Main entry point"""
    # Setup default logging (will be overridden by colored logger)
    logging.basicConfig(
        level=logging.INFO,
        format='%(asctime)s - %(name)s - %(levelname)s - %(message)s'
    )

    # Create main logger
    main_logger = get_colored_logger("main")

    # Create detailed help description
    detailed_help = """arguments:
  deploy
    deploy ai <model_path> [version]
      model_path: Path to AI model package (.pkg)
      version: Optional model version string
      example: edgeapp_cli deploy ai model.pkg v1.0.0

    deploy edge_app <wasm_path> [module_name]
      wasm_path: Path to Edge App WASM file (.wasm)
      module_name: Optional module name
      example: edgeapp_cli deploy edge_app app.wasm my_module

  undeploy
    undeploy
      No positional arguments. Undeploys all Edge App modules.

  send Configuration
    send conf [config_file] [instance_id] [ps=value]
      config_file: Optional path to configuration JSON (default: configuration.json)
      instance_id: Optional Edge App instance UUID (if omitted, CLI tries to auto-detect)
      ps=value: Optional process_state value (ps=0/1/2) or use --process-state
      example: edgeapp_cli send conf configuration.json my_instance_id ps=1
"""

    # Custom formatter to remove metavar and improve layout
    class CustomHelpFormatter(argparse.RawDescriptionHelpFormatter):
        def _format_action_invocation(self, action):
            if action.option_strings:
                # For optional arguments, only show the option strings without metavar
                return ', '.join(action.option_strings)
            else:
                # For positional arguments, use default behavior
                return super()._format_action_invocation(action)

        def _format_action(self, action):
            # Get the invocation string (e.g., '--mqtt-host')
            invocation = self._format_action_invocation(action)

            # Skip help formatting for options if they have option strings
            if action.option_strings:
                if action.help:
                    # Format as: "  --option-name    description"
                    return f"  {invocation:<12} {action.help}\n"
                else:
                    return f"  {invocation}\n"
            else:
                # Use default formatting for positional arguments
                return super()._format_action(action)

    parser = argparse.ArgumentParser(
        description=detailed_help,
        usage=argparse.SUPPRESS,
        formatter_class=CustomHelpFormatter,
        add_help=False  # Disable default help to avoid showing positional arguments
    )
    # Add custom help argument
    parser.add_argument('-h', '--help', action='help', default=argparse.SUPPRESS,
                       help='show this help message and exit')
    parser.add_argument("--mqtt-host", default="localhost", metavar="", help="MQTT broker host")
    parser.add_argument("--mqtt-port", type=int, default=1883, metavar="", help="MQTT broker port")
    parser.add_argument("--http-port", type=int, default=8000, metavar="", help="HTTP server port")
    parser.add_argument("--http-host", default=None, metavar="",
                       help="HTTP server host address (default: auto-detect local IP)")

    # Add subcommands with add_help=False to suppress their positional arguments display
    subparsers = parser.add_subparsers(dest='command', help=argparse.SUPPRESS)

    # Add 'deploy' command
    deploy_parser = subparsers.add_parser('deploy', help='Deploy AI models or Edge Apps')
    deploy_subparsers = deploy_parser.add_subparsers(dest='deploy_command', help='Deploy subcommands')

    # Add 'deploy ai' command
    ai_parser = deploy_subparsers.add_parser('ai', help='Deploy AI model')
    ai_parser.add_argument('model_path', help='Path to AI model file')
    ai_parser.add_argument('version', nargs='?', help='AI model version (optional)')

    # Add 'deploy edge_app' command
    edge_app_parser = deploy_subparsers.add_parser('edge_app', help='Deploy Edge App module')
    edge_app_parser.add_argument('wasm_path', help='Path to Edge App WASM file')
    edge_app_parser.add_argument('module_name', nargs='?', help='Module name (optional)')

    # Add 'undeploy' command
    undeploy_parser = subparsers.add_parser('undeploy', help='Undeploy all Edge App modules')

    # Add 'send' command
    send_parser = subparsers.add_parser('send', help='Send commands')
    send_subparsers = send_parser.add_subparsers(dest='send_command', help='Send subcommands')

    # Add 'send conf' command
    conf_parser = send_subparsers.add_parser('conf', help='Send configuration to Edge App')
    conf_parser.add_argument('args', nargs='*',
                           help='Arguments: [config_file] [instance_id] [ps=value]')
    conf_parser.add_argument('--instance', default='auto',
                           help='Edge App instance ID (default: auto-detect deployed instance)')
    conf_parser.add_argument('--topic', default='edge_app',
                           help='Configuration topic (default: edge_app)')
    conf_parser.add_argument('--process-state', type=int, choices=[0, 1, 2],
                           help='Set process_state (0=stop, 1=start, 2=restart)')

    args = parser.parse_args()

    # Handle command-line argument parsing for send conf command
    if args.command == 'send' and args.send_command == 'conf':
        # Parse positional arguments: [config_file] [instance_id] [ps=value]
        config_file = 'configuration.json'  # Default
        instance = 'auto'  # Default
        process_state = args.process_state  # From --process-state flag

        for arg in args.args:
            if arg.startswith('ps='):
                # Handle ps=value format
                try:
                    ps_value = int(arg.split('=')[1])
                    if ps_value in [0, 1, 2]:
                        process_state = ps_value
                    else:
                        main_logger.error(f"Invalid ps value: {ps_value}. Must be 0, 1, 2")
                        return 1
                except (ValueError, IndexError):
                    main_logger.error(f"Invalid ps format: {arg}. Use ps=0, ps=1, or ps=2")
                    return 1
            elif len(arg) > 20 and '-' in arg:
                # Likely an instance ID (UUID format like: 5ad9c7f6-cac0-46da-8007-8e7c5ff99ef7)
                instance = arg
            elif arg.endswith('.json'):
                # This is a configuration file
                config_file = arg
            else:
                # Check if it looks like a config file (no dashes, shorter length)
                if len(arg) < 20 and '-' not in arg:
                    config_file = arg
                else:
                    # Default to instance ID for longer strings with dashes
                    instance = arg

        # Update args object with parsed values
        args.config_file = config_file
        args.instance = instance
        args.process_state = process_state

    # Create and run CLI with HTTP host configuration
    cli = DeploymentCLI(http_host=args.http_host)

    try:
        # Determine if this is command line mode and skip device initialization for all commands
        skip_device_init = bool(args.command)  # Skip device init for any command line mode

        # Start services
        if not cli.start_services(args.mqtt_host, args.mqtt_port, args.http_port, skip_device_init):
            return 1

        # Handle command line mode vs interactive mode
        if args.command:
            # Command line mode
            if args.command == 'deploy':
                # For command line mode, skip device initialization check for faster operation
                main_logger.info("Command line mode: Skipping device initialization for all commands")

                # Check if HTTP server is running before deployment
                if not cli.http_server_running:
                    main_logger.warning("HTTP server is not running. Files may not be accessible for deployment.")
                    main_logger.info("Attempting to start HTTP server...")
                    if not cli.start_http_server():
                        main_logger.error("Failed to start HTTP server. Deployment may fail.")
                        return 1

                if args.deploy_command == 'ai':
                    result = cli.deploy_ai_model(args.model_path, args.version)
                    return 0 if result else 1
                elif args.deploy_command == 'edge_app':
                    result = cli.deploy_wasm(args.wasm_path, args.module_name)
                    return 0 if result else 1
                else:
                    main_logger.error("Invalid deploy command. Use 'deploy ai' or 'deploy edge_app'")
                    return 1
            elif args.command == 'undeploy':
                main_logger.info("Command line mode: Undeploying all Edge App modules")
                result = cli.undeploy_wasm()
                return 0 if result else 1
            elif args.command == 'send' and args.send_command == 'conf':
                result = cli.send_configuration(
                    args.config_file,
                    args.instance,
                    args.process_state,
                    args.topic
                )
                return 0 if result else 1
            else:
                main_logger.error(f"Unknown command: {args.command}")
                return 1
        else:
            # Interactive mode
            cli.run_cli()

    except KeyboardInterrupt:
        main_logger.info("Received shutdown signal, terminating...")
    finally:
        main_logger.info("Cleaning up resources...")
        cli.stop_services()
        # Save command history on exit
        main_logger.info("Saving final command history...")
        save_history()

        # Show different messages for command mode vs interactive mode
        if args.command:
            # Command mode - show success message for command execution
            if args.command == 'deploy':
                if args.deploy_command == 'ai':
                    main_logger.success("AI model deployment completed successfully")
                elif args.deploy_command == 'edge_app':
                    main_logger.success("Edge App deployment completed successfully")
                else:
                    main_logger.success("Deploy command completed successfully")
            elif args.command == 'undeploy':
                main_logger.success("Undeploy command completed successfully")
            elif args.command == 'send' and args.send_command == 'conf':
                main_logger.success("Configuration send command completed successfully")
            else:
                main_logger.success(f"Command '{args.command}' completed successfully")
        else:
            # Interactive mode - show goodbye message
            main_logger.success("Shutdown sequence finished. Goodbye!")

    return 0


if __name__ == "__main__":
    exit(main())
