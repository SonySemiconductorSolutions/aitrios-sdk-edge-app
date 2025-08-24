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
Test utilities and helpers for Deploy CLI tests
"""

import json
import tempfile
import time
from pathlib import Path
from unittest.mock import Mock, MagicMock
from uuid import uuid4


class MockMQTTBroker:
    """Mock MQTT broker for testing"""

    def __init__(self):
        self.connected = False
        self.recent_messages = []
        self.received_attributes = {}
        self.deployment_status_callbacks = {}
        self.ai_model_status_callbacks = {}
        self.verbose_logging = False
        self.client = Mock()
        self.client.publish.return_value.rc = 0  # Default success

    def connect(self):
        self.connected = True
        return True

    def disconnect(self):
        self.connected = False

    def broker_log_on(self):
        self.verbose_logging = True

    def broker_log_off(self):
        self.verbose_logging = False

    def wait_for_ai_model_config_response(self, req_id, timeout=60):
        """Mock AI model response with success"""
        return True, None

    def wait_for_deployment_status(self, deployment_id, timeout=60):
        """Mock deployment response with success"""
        return True, None

    def show_recent_messages(self):
        pass


class MockHTTPServer:
    """Mock HTTP server for testing"""

    def __init__(self, port=8000):
        self.port = port
        self.running = False

    def start(self):
        self.running = True
        return True

    def stop(self):
        self.running = False


class TestFileHelper:
    """Helper for creating test files"""

    @staticmethod
    def create_temp_file(content=b"test data", suffix=".bin", filename=None):
        """Create a temporary file with specified content"""
        temp_dir = tempfile.mkdtemp()
        if filename:
            file_path = Path(temp_dir) / filename
        else:
            file_path = Path(temp_dir) / f"test_{uuid4().hex[:8]}{suffix}"

        file_path.write_bytes(content)
        return file_path, temp_dir

    @staticmethod
    def create_ai_model_file(size_kb=100):
        """Create a fake AI model file"""
        content = b"AI_MODEL_DATA" * (size_kb * 1024 // 13)  # Approximate size
        return TestFileHelper.create_temp_file(content, ".bin", "test_model.bin")

    @staticmethod
    def create_wasm_file(size_kb=50):
        """Create a fake WASM file"""
        content = b"WASM_MODULE" * (size_kb * 1024 // 11)  # Approximate size
        return TestFileHelper.create_temp_file(content, ".wasm", "test_module.wasm")


class MQTTMessageBuilder:
    """Helper for building MQTT messages"""

    @staticmethod
    def ai_model_progress_message(progress=50, state="downloading", chip="sensor_chip"):
        """Build AI model progress message"""
        return {
            "state/PRIVATE_deploy_ai_model_123": json.dumps({
                "targets": [
                    {
                        "chip": chip,
                        "progress": progress,
                        "process_state": state,
                        "hash": "abc123def456",
                        "size": 1024000
                    }
                ]
            })
        }

    @staticmethod
    def ai_model_response_message(req_id, code=0, progress=100, state="done"):
        """Build AI model response message"""
        return {
            f"state/PRIVATE_deploy_ai_model_{req_id}": json.dumps({
                "res_info": {
                    "res_id": f"response_{req_id}",
                    "code": code
                },
                "targets": [
                    {
                        "chip": "sensor_chip",
                        "progress": progress,
                        "process_state": state,
                        "hash": "abc123def456",
                        "size": 1024000
                    }
                ]
            })
        }

    @staticmethod
    def deployment_status_message(deployment_id, status="ok", message="Success"):
        """Build deployment status message"""
        return {
            "deploymentStatus": {
                "deploymentId": deployment_id,
                "reconcileStatus": status,
                "message": message
            }
        }


class TestScenarios:
    """Pre-built test scenarios"""

    @staticmethod
    def successful_ai_deployment():
        """Scenario for successful AI model deployment"""
        return {
            "mqtt_responses": [
                MQTTMessageBuilder.ai_model_progress_message(25, "downloading"),
                MQTTMessageBuilder.ai_model_progress_message(50, "downloading"),
                MQTTMessageBuilder.ai_model_progress_message(75, "installing"),
                MQTTMessageBuilder.ai_model_progress_message(100, "done")
            ],
            "expected_result": True
        }

    @staticmethod
    def failed_ai_deployment():
        """Scenario for failed AI model deployment"""
        return {
            "mqtt_responses": [
                MQTTMessageBuilder.ai_model_progress_message(25, "downloading"),
                MQTTMessageBuilder.ai_model_response_message("test", code=1, progress=30, state="error")
            ],
            "expected_result": False
        }

    @staticmethod
    def successful_wasm_deployment():
        """Scenario for successful WASM deployment"""
        return {
            "mqtt_responses": [
                MQTTMessageBuilder.deployment_status_message("test_deployment", "ok", "Deployment successful")
            ],
            "expected_result": True
        }


class LogCapture:
    """Helper for capturing log messages in tests"""

    def __init__(self):
        self.messages = []
        self.errors = []
        self.warnings = []

    def info(self, message):
        self.messages.append(("INFO", message))

    def error(self, message):
        self.messages.append(("ERROR", message))
        self.errors.append(message)

    def warning(self, message):
        self.messages.append(("WARNING", message))
        self.warnings.append(message)

    def success(self, message):
        self.messages.append(("SUCCESS", message))

    def tip(self, message):
        self.messages.append(("TIP", message))

    def clear(self):
        self.messages.clear()
        self.errors.clear()
        self.warnings.clear()

    def get_messages(self, level=None):
        if level:
            return [msg[1] for msg in self.messages if msg[0] == level]
        return [msg[1] for msg in self.messages]


class MockCLIEnvironment:
    """Complete mock environment for CLI testing"""

    def __init__(self):
        self.temp_dir = tempfile.mkdtemp()
        self.upload_dir = Path(self.temp_dir) / "server_dir"
        self.upload_dir.mkdir(exist_ok=True)

        self.mqtt_broker = MockMQTTBroker()
        self.log_capture = LogCapture()

    def cleanup(self):
        """Clean up temporary files"""
        import shutil
        shutil.rmtree(self.temp_dir, ignore_errors=True)

    def create_test_cli(self):
        """Create a CLI instance with mocked dependencies"""
        from edgeapp_cli import DeploymentCLI

        cli = DeploymentCLI()
        cli.upload_dir = self.upload_dir
        cli.mqtt_broker = self.mqtt_broker
        cli.logger = self.log_capture
        cli.http_server_running = True  # Mock HTTP server as running

        return cli


# Test decorators
def requires_mqtt(func):
    """Decorator to skip tests that require MQTT broker"""
    import unittest
    return unittest.skipUnless(
        False,  # Set to True if you have a test MQTT broker
        "MQTT broker not available for testing"
    )(func)


def requires_http_server(func):
    """Decorator to skip tests that require HTTP server"""
    import unittest
    return unittest.skipUnless(
        False,  # Set to True if you want to test with real HTTP server
        "HTTP server testing not enabled"
    )(func)
