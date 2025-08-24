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

"""OnWire schema interface for MQTT communication"""

class OnWireSchema:
    """Schema definition for wire protocol"""

    def __init__(self, version: str = "evp2"):
        self.version = version
        self.schema_type = version

    def to_deployment(self, deployment_id, modules):
        """Create deployment message"""
        return {
            "deployment": {
                "deploymentId": str(deployment_id),
                "instanceSpecs": {
                    str(module.instance): {
                        "name": str(module.name),
                        "moduleId": str(module.id),
                        "publish": {},
                        "subscribe": {},
                    } for module in modules
                },
                "modules": {
                    str(module.id): {
                        "entryPoint": module.entry_point,
                        "moduleImpl": module.type,
                        "downloadUrl": module.host,
                        "hash": module.hash,
                    } for module in modules
                },
                "publishTopics": {},
                "subscribeTopics": {},
            }
        }

    def to_config(self, reqid, instance, topic, config):
        """Create configuration message"""
        import json
        return {
            f"configuration/{instance}/{topic}": json.dumps(config)
        }

    def to_rpc(self, reqid, instance, method, params):
        """Create RPC message"""
        return {
            "method": method,
            "params": params,
            "reqId": str(reqid)
        }
