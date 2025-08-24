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
Test suite for the AI Model and WASM Deployment CLI Tool
Uses unittest for compatibility without external dependencies
"""

import json
import os
import tempfile
import threading
import time
import unittest
from pathlib import Path
from unittest.mock import Mock, MagicMock, patch, call
from uuid import uuid4

# Import the module under test
from edgeapp_cli import DeploymentCLI, SimpleMQTTBroker, get_local_ip
from src.ai_models import AIModel
from src.modules import Module


class TestGetLocalIP(unittest.TestCase):
    """Test local IP address detection"""

    def test_get_local_ip_returns_string(self):
        """Test that get_local_ip returns a string"""
        ip = get_local_ip()
        self.assertIsInstance(ip, str)
        self.assertNotEqual(ip, "")

    @patch('socket.socket')
    def test_get_local_ip_fallback_to_localhost(self, mock_socket):
        """Test fallback to localhost when socket fails"""
        mock_socket.side_effect = Exception("Network error")
        ip = get_local_ip()
        self.assertEqual(ip, "localhost")


class TestSimpleMQTTBroker(unittest.TestCase):
    """Test MQTT broker functionality"""

    def setUp(self):
        """Set up test fixtures"""
        self.broker = SimpleMQTTBroker()

    def test_mqtt_broker_initialization(self):
        """Test MQTT broker initialization"""
        self.assertEqual(self.broker.host, "localhost")
        self.assertEqual(self.broker.port, 1883)
        self.assertFalse(self.broker.connected)
        self.assertEqual(self.broker.recent_messages, [])
        self.assertFalse(self.broker.verbose_logging)

    def test_broker_log_on_off(self):
        """Test enabling and disabling verbose logging"""
        # Initially off
        self.assertFalse(self.broker.verbose_logging)

        # Turn on
        self.broker.broker_log_on()
        self.assertTrue(self.broker.verbose_logging)

        # Turn off
        self.broker.broker_log_off()
        self.assertFalse(self.broker.verbose_logging)

    @patch('paho.mqtt.client.Client')
    def test_connect_success(self, mock_client_class):
        """Test successful MQTT connection"""
        mock_client = Mock()
        mock_client_class.return_value = mock_client

        # Create a new broker instance to use the mocked client
        broker = SimpleMQTTBroker()
        broker.client = mock_client

        # Mock successful connection behavior
        def simulate_connection(*args, **kwargs):
            broker.connected = True

        # Set up the mock to simulate connection
        mock_client.connect.side_effect = simulate_connection

        result = broker.connect()
        self.assertTrue(result)
        mock_client.connect.assert_called_once_with("localhost", 1883, 60)
        mock_client.loop_start.assert_called_once()

    @patch('paho.mqtt.client.Client')
    def test_connect_failure(self, mock_client_class):
        """Test MQTT connection failure"""
        mock_client = Mock()
        mock_client_class.return_value = mock_client

        # Create a new broker instance to use the mocked client
        broker = SimpleMQTTBroker()
        broker.client = mock_client

        # Mock connection failure
        mock_client.connect.side_effect = Exception("Connection failed")

        result = broker.connect()
        self.assertFalse(result)

    def test_on_message_processing(self):
        """Test message processing and storage"""
        # Create a mock message
        mock_msg = Mock()
        mock_msg.topic = "test/topic"
        mock_msg.payload.decode.return_value = '{"test": "data"}'

        # Process the message
        self.broker._on_message(None, None, mock_msg)

        # Check if message was stored
        self.assertEqual(len(self.broker.recent_messages), 1)
        self.assertEqual(self.broker.recent_messages[0]["topic"], "test/topic")
        self.assertIn("payload", self.broker.recent_messages[0])
        self.assertIn("timestamp", self.broker.recent_messages[0])

    def test_ai_model_progress_parsing(self):
        """Test AI model progress parsing from state data"""
        state_data = {
            "targets": [
                {
                    "chip": "sensor_chip",
                    "progress": 75,
                    "process_state": "downloading"
                }
            ]
        }

        # Enable verbose logging to capture progress messages
        self.broker.verbose_logging = True

        with patch.object(self.broker._logger, 'debug') as mock_logger:
            self.broker._check_ai_model_progress(state_data)
            mock_logger.assert_called_with(
                "AI Model Progress - Chip: sensor_chip, Progress: 75%, State: downloading"
            )

    def test_wait_for_ai_model_config_response_success(self):
        """Test successful AI model configuration response"""
        req_id = str(uuid4())

        # Create a mock callback that simulates successful deployment
        def simulate_success():
            time.sleep(0.1)  # Brief delay
            # Simulate the callback being called with success data
            res_info = {"code": 0}
            state_data = {
                "targets": [
                    {
                        "chip": "sensor_chip",
                        "progress": 100,
                        "process_state": "done"
                    }
                ]
            }
            if req_id in self.broker.ai_model_status_callbacks:
                self.broker.ai_model_status_callbacks[req_id](res_info, state_data)

        # Start the simulation in a separate thread
        thread = threading.Thread(target=simulate_success)
        thread.start()

        # Test the wait function
        success, error = self.broker.wait_for_ai_model_config_response(req_id, timeout=5)

        thread.join()
        self.assertTrue(success)
        self.assertIsNone(error)

    def test_wait_for_ai_model_config_response_timeout(self):
        """Test AI model configuration response timeout"""
        req_id = str(uuid4())

        # Test timeout (use very short timeout for fast test)
        success, error = self.broker.wait_for_ai_model_config_response(req_id, timeout=1)

        self.assertFalse(success)
        self.assertIn("Timeout", error)


class TestDeploymentCLI(unittest.TestCase):
    """Test DeploymentCLI functionality"""

    def setUp(self):
        """Set up test fixtures"""
        self.temp_dir = tempfile.mkdtemp()
        self.cli = DeploymentCLI()
        self.cli.upload_dir = Path(self.temp_dir) / "server_dir"
        self.cli.upload_dir.mkdir(exist_ok=True)

    def tearDown(self):
        """Clean up test fixtures"""
        import shutil
        shutil.rmtree(self.temp_dir, ignore_errors=True)

    def test_cli_initialization(self):
        """Test CLI initialization"""
        cli = DeploymentCLI()
        self.assertIsNotNone(cli.logger)
        self.assertIsNone(cli.mqtt_broker)
        self.assertEqual(cli.deployed_ai_models, [])
        self.assertEqual(cli.deployed_modules, [])
        self.assertEqual(cli.http_port, 8000)

    def test_cli_initialization_with_custom_host(self):
        """Test CLI initialization with custom HTTP host"""
        custom_host = "192.168.1.100"
        cli = DeploymentCLI(http_host=custom_host)
        self.assertEqual(cli.http_host, custom_host)

    def test_calculate_file_hash(self):
        """Test file hash calculation"""
        # Create a test file
        test_file = Path(self.temp_dir) / "test.txt"
        test_content = b"Hello, World!"
        test_file.write_bytes(test_content)

        # Calculate hash for WASM file (returns hex)
        hash_value = self.cli.calculate_file_hash_wasm(test_file)

        # Verify hash
        import hashlib
        expected_hash = hashlib.sha256(test_content).hexdigest()
        self.assertEqual(hash_value, expected_hash)

    def test_copy_file_to_upload_dir(self):
        """Test copying file to upload directory"""
        # Create a test file
        test_file = Path(self.temp_dir) / "test.txt"
        test_content = b"Test file content"
        test_file.write_bytes(test_content)

        # Copy file
        url, dest_path = self.cli.copy_file_to_upload_dir(test_file)

        # Verify file was copied
        self.assertTrue(dest_path.exists())
        self.assertEqual(dest_path.read_bytes(), test_content)
        self.assertIn("test.txt", url)
        self.assertIn(str(self.cli.http_port), url)

    def test_copy_nonexistent_file(self):
        """Test copying non-existent file raises error"""
        nonexistent_file = Path(self.temp_dir) / "nonexistent.txt"

        with self.assertRaises(FileNotFoundError):
            self.cli.copy_file_to_upload_dir(nonexistent_file)

    @patch('edgeapp_cli.SimpleMQTTBroker')
    def test_start_services_success(self, mock_broker_class):
        """Test successful service startup"""
        mock_broker = Mock()
        mock_broker.connect.return_value = True
        mock_broker_class.return_value = mock_broker

        with patch.object(self.cli, 'start_http_server') as mock_http:
            mock_http.return_value = True

            result = self.cli.start_services()
            self.assertTrue(result)
            mock_broker.connect.assert_called_once()

    @patch('edgeapp_cli.SimpleMQTTBroker')
    def test_start_services_mqtt_failure(self, mock_broker_class):
        """Test service startup with MQTT failure"""
        mock_broker = Mock()
        mock_broker.connect.return_value = False
        mock_broker_class.return_value = mock_broker

        result = self.cli.start_services()
        self.assertFalse(result)

    def test_list_deployed_empty(self):
        """Test listing deployed items when none exist"""
        with patch.object(self.cli.logger, 'info') as mock_logger:
            self.cli.list_deployed()

            # Check that it logs about no deployed items
            calls = mock_logger.call_args_list
            self.assertTrue(any("None" in str(call) for call in calls))

    def test_list_deployed_with_items(self):
        """Test listing deployed items when some exist"""
        # Add a mock AI model
        mock_model = Mock()
        mock_model.name = "test_model"
        mock_model.version = "test_version"
        self.cli.deployed_ai_models.append(mock_model)

        # Add a mock WASM module
        mock_module = Mock()
        mock_module.name = "test_module"
        mock_module.instance = "test_instance_id"
        self.cli.deployed_modules.append(mock_module)

        # Mock the detection methods to return False (not detected from device)
        with patch.object(self.cli, '_is_ai_model_detected_from_device', return_value=False), \
             patch.object(self.cli, '_get_ai_models_from_device', return_value=[]), \
             patch.object(self.cli, '_get_wasm_instances_from_device', return_value=[]), \
             patch.object(self.cli.logger, 'info') as mock_info, \
             patch.object(self.cli.logger, 'warning') as mock_warning:

            self.cli.list_deployed()

            # Check that it logs the deployed items
            warning_calls = [str(call) for call in mock_warning.call_args_list]
            # Should show AI model as "managed by CLI"
            self.assertTrue(any("test_version" in call and "managed by CLI" in call for call in warning_calls))
            # Should show WASM instance as "managed by CLI"
            self.assertTrue(any("test_instance_id" in call and "managed by CLI" in call for call in warning_calls))

    def test_list_deployed_with_mqtt_detection(self):
        """Test listing deployed items with MQTT-detected models"""
        # Set up mock MQTT broker with received attributes
        self.cli.mqtt_broker = Mock()
        self.cli.mqtt_broker.received_attributes = {
            "state/PRIVATE_deploy_ai_model_123": json.dumps({
                "targets": [
                    {
                        "progress": 100,
                        "process_state": "done",
                        "hash": "abc123def456",
                        "size": 1024000
                    }
                ]
            })
        }

        # Mock the detection methods to return the expected model
        mock_model_info = {
            "hash": "abc123def456",
            "size": 1024000,
            "version": "model_abc123de"
        }

        with patch.object(self.cli, '_get_ai_models_from_device', return_value=[mock_model_info]), \
             patch.object(self.cli, '_get_wasm_instances_from_device', return_value=[]), \
             patch.object(self.cli.logger, 'info') as mock_info, \
             patch.object(self.cli.logger, 'success') as mock_success:

            self.cli.list_deployed()

            # Check that it logs the detected AI model
            success_calls = [str(call) for call in mock_success.call_args_list]
            self.assertTrue(any("model_abc123de" in call and "detected from device" in call for call in success_calls))

    @patch('socket.socket')
    def test_show_server_status(self, mock_socket):
        """Test showing server status"""
        # Mock port checking
        mock_socket_instance = Mock()
        mock_socket.return_value.__enter__.return_value = mock_socket_instance
        mock_socket_instance.bind.side_effect = OSError()  # Port in use

        with patch.object(self.cli.logger, 'info') as mock_logger:
            self.cli.show_server_status()

            # Check that status information is logged
            calls = [str(call) for call in mock_logger.call_args_list]
            self.assertTrue(any("Server Status" in call for call in calls))


class TestDeploymentFunctionality(unittest.TestCase):
    """Test deployment functionality with mocked dependencies"""

    def setUp(self):
        """Set up test fixtures"""
        self.temp_dir = tempfile.mkdtemp()
        self.cli = DeploymentCLI()
        self.cli.upload_dir = Path(self.temp_dir) / "server_dir"
        self.cli.upload_dir.mkdir(exist_ok=True)

        # Mock MQTT broker
        self.mock_broker = Mock()
        self.cli.mqtt_broker = self.mock_broker

    def tearDown(self):
        """Clean up test fixtures"""
        import shutil
        shutil.rmtree(self.temp_dir, ignore_errors=True)

    def test_deploy_ai_model_success(self):
        """Test successful AI model deployment"""
        # Create a test AI model file
        model_file = Path(self.temp_dir) / "test_model.bin"
        model_file.write_bytes(b"fake AI model data")

        # Mock successful deployment response
        self.mock_broker.wait_for_ai_model_config_response.return_value = (True, None)
        self.mock_broker.client.publish.return_value.rc = 0  # Success
        self.mock_broker.verbose_logging = False

        # Mock the onwire_schema.to_config to return a serializable dict
        self.mock_broker.onwire_schema.to_config.return_value = {
            "reqId": "test_req_id",
            "instance": "$system",
            "topic": "PRIVATE_deploy_ai_model",
            "config": {
                "targets": [{"chip": "sensor_chip", "hash": "test_hash"}],
                "req_info": {"req_id": "test_req_id"}
            }
        }

        # Mock webserver log suppression
        with patch('webserver.CustomHTTPRequestHandler.set_log_suppression'):
            result = self.cli.deploy_ai_model(str(model_file), "test_version_123")

        self.assertTrue(result)
        self.assertEqual(len(self.cli.deployed_ai_models), 1)
        self.assertEqual(self.cli.deployed_ai_models[0].version, "test_version_123")

    def test_deploy_ai_model_file_not_found(self):
        """Test AI model deployment with non-existent file"""
        result = self.cli.deploy_ai_model("nonexistent_model.bin")
        self.assertFalse(result)
        self.assertEqual(len(self.cli.deployed_ai_models), 0)

    def test_deploy_ai_model_mqtt_failure(self):
        """Test AI model deployment with MQTT failure"""
        # Create a test AI model file
        model_file = Path(self.temp_dir) / "test_model.bin"
        model_file.write_bytes(b"fake AI model data")

        # Mock MQTT publish failure
        self.mock_broker.client.publish.return_value.rc = 1  # Failure
        self.mock_broker.verbose_logging = False

        with patch('webserver.CustomHTTPRequestHandler.set_log_suppression'):
            result = self.cli.deploy_ai_model(str(model_file))

        self.assertFalse(result)
        self.assertEqual(len(self.cli.deployed_ai_models), 0)

    def test_deploy_ai_model_deployment_failure(self):
        """Test AI model deployment with deployment failure"""
        # Create a test AI model file
        model_file = Path(self.temp_dir) / "test_model.bin"
        model_file.write_bytes(b"fake AI model data")

        # Mock deployment failure response
        self.mock_broker.wait_for_ai_model_config_response.return_value = (False, "Deployment failed")
        self.mock_broker.client.publish.return_value.rc = 0
        self.mock_broker.verbose_logging = False

        with patch('webserver.CustomHTTPRequestHandler.set_log_suppression'):
            result = self.cli.deploy_ai_model(str(model_file))

        self.assertFalse(result)
        self.assertEqual(len(self.cli.deployed_ai_models), 0)

    def test_deploy_wasm_success(self):
        """Test successful WASM module deployment"""
        # Create a test WASM file
        wasm_file = Path(self.temp_dir) / "test_module.wasm"
        wasm_file.write_bytes(b"fake WASM module data")

        # Mock successful deployment response
        self.mock_broker.wait_for_deployment_status.return_value = (True, None)
        self.mock_broker.client.publish.return_value.rc = 0
        self.mock_broker.verbose_logging = False

        # Mock the OnWireSchema to_deployment method to return a valid deployment message
        mock_deployment = {
            "deployment": {
                "deploymentId": "test-deployment-id",
                "instanceSpecs": {},
                "modules": {},
                "publishTopics": {},
                "subscribeTopics": {}
            }
        }
        self.mock_broker.onwire_schema.to_deployment.return_value = mock_deployment

        # Mock the instance detection method to avoid iteration issues
        with patch.object(self.cli, '_get_deployed_wasm_instance_by_hash', return_value="test_instance_id"), \
             patch('webserver.CustomHTTPRequestHandler.set_log_suppression'):
            result = self.cli.deploy_wasm(str(wasm_file), "test_module")

        self.assertTrue(result)
        self.assertEqual(len(self.cli.deployed_modules), 1)
        self.assertEqual(self.cli.deployed_modules[0].name, "test_module")

    def test_deploy_wasm_file_not_found(self):
        """Test WASM deployment with non-existent file"""
        result = self.cli.deploy_wasm("nonexistent_module.wasm")
        self.assertFalse(result)
        self.assertEqual(len(self.cli.deployed_modules), 0)


class TestMQTTMessageHandling(unittest.TestCase):
    """Test MQTT message handling and callback functionality"""

    def setUp(self):
        """Set up test fixtures"""
        self.broker = SimpleMQTTBroker()

    def test_check_ai_model_status_with_state_data(self):
        """Test AI model status checking with state data"""
        # Mock a callback
        req_id = "test_req_id"
        callback_called = False
        callback_args = {}

        def mock_callback(res_info, state_data):
            nonlocal callback_called, callback_args
            callback_called = True
            callback_args = {"res_info": res_info, "state_data": state_data}

        self.broker.ai_model_status_callbacks[req_id] = mock_callback

        # Create test payload
        payload_data = {
            "state/PRIVATE_deploy_ai_model_123": json.dumps({
                "res_info": {
                    "res_id": "response_123",
                    "code": 0
                },
                "targets": [
                    {
                        "chip": "sensor_chip",
                        "progress": 100,
                        "process_state": "done"
                    }
                ]
            })
        }

        # Process the payload
        self.broker._check_ai_model_status(payload_data)

        # Verify callback was called
        self.assertTrue(callback_called)
        self.assertIn("res_info", callback_args)
        self.assertIn("state_data", callback_args)
        self.assertEqual(callback_args["res_info"]["code"], 0)

    def test_check_deployment_status_callback(self):
        """Test deployment status callback functionality"""
        deployment_id = "test_deployment_id"
        callback_called = False
        callback_status = None

        def mock_callback(status):
            nonlocal callback_called, callback_status
            callback_called = True
            callback_status = status

        self.broker.deployment_status_callbacks[deployment_id] = mock_callback

        # Create test payload
        payload_data = {
            "deploymentStatus": {
                "deploymentId": deployment_id,
                "reconcileStatus": "ok",
                "message": "Deployment successful"
            }
        }

        # Process the payload
        self.broker._check_deployment_status(payload_data)

        # Verify callback was called
        self.assertTrue(callback_called)
        self.assertEqual(callback_status["reconcileStatus"], "ok")


class TestErrorHandling(unittest.TestCase):
    """Test error handling and edge cases"""

    def setUp(self):
        """Set up test fixtures"""
        self.broker = SimpleMQTTBroker()
        self.cli = DeploymentCLI()

    def test_mqtt_message_with_invalid_json(self):
        """Test handling of MQTT messages with invalid JSON"""
        mock_msg = Mock()
        mock_msg.topic = "test/topic"
        mock_msg.payload.decode.return_value = "invalid json {"

        # This should not raise an exception
        self.broker._on_message(None, None, mock_msg)

        # Message should still be stored with the invalid JSON
        self.assertEqual(len(self.broker.recent_messages), 1)

    def test_callback_exception_handling(self):
        """Test that callback exceptions are handled gracefully"""
        req_id = "test_req_id"

        def failing_callback(res_info, state_data):
            raise Exception("Callback error")

        self.broker.ai_model_status_callbacks[req_id] = failing_callback

        # This should not raise an exception
        with patch.object(self.broker._logger, 'error') as mock_logger:
            payload_data = {
                "state/PRIVATE_deploy_ai_model_123": json.dumps({
                    "res_info": {"res_id": "test", "code": 0}
                })
            }
            self.broker._check_ai_model_status(payload_data)

            # Error should be logged
            mock_logger.assert_called()

    def test_show_recent_messages_empty(self):
        """Test showing recent messages when none exist"""
        with patch.object(self.broker._logger, 'error') as mock_logger:
            self.broker.show_recent_messages()
            mock_logger.assert_called_with("No recent messages")


if __name__ == "__main__":
    # Run the tests
    unittest.main(verbosity=2)
