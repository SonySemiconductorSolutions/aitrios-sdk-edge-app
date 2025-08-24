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
Functional tests for Deploy CLI with proper import handling
"""

import unittest
import tempfile
import os
import sys
import json
from pathlib import Path
from unittest.mock import Mock, patch, MagicMock

# Add the parent directory to Python path
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))

# Try to import the CLI components
try:
    from edgeapp_cli import get_local_ip
    GET_LOCAL_IP_AVAILABLE = True
except ImportError as e:
    print(f"Warning: Could not import get_local_ip: {e}")
    GET_LOCAL_IP_AVAILABLE = False

try:
    from edgeapp_cli import SimpleMQTTBroker
    MQTT_BROKER_AVAILABLE = True
except ImportError as e:
    print(f"Warning: Could not import SimpleMQTTBroker: {e}")
    MQTT_BROKER_AVAILABLE = False

try:
    from edgeapp_cli import DeploymentCLI
    DEPLOYMENT_CLI_AVAILABLE = True
except ImportError as e:
    print(f"Warning: Could not import DeploymentCLI: {e}")
    DEPLOYMENT_CLI_AVAILABLE = False


class TestGetLocalIP(unittest.TestCase):
    """Test local IP address detection"""

    @unittest.skipUnless(GET_LOCAL_IP_AVAILABLE, "get_local_ip not available")
    def test_get_local_ip_returns_string(self):
        """Test that get_local_ip returns a string"""
        ip = get_local_ip()
        self.assertIsInstance(ip, str)
        self.assertNotEqual(ip, "")

    @unittest.skipUnless(GET_LOCAL_IP_AVAILABLE, "get_local_ip not available")
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
        if MQTT_BROKER_AVAILABLE:
            self.broker = SimpleMQTTBroker()

    @unittest.skipUnless(MQTT_BROKER_AVAILABLE, "SimpleMQTTBroker not available")
    def test_mqtt_broker_initialization(self):
        """Test MQTT broker initialization"""
        self.assertEqual(self.broker.host, "localhost")
        self.assertEqual(self.broker.port, 1883)
        self.assertFalse(self.broker.connected)
        self.assertEqual(self.broker.recent_messages, [])
        self.assertFalse(self.broker.verbose_logging)

    @unittest.skipUnless(MQTT_BROKER_AVAILABLE, "SimpleMQTTBroker not available")
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

    @unittest.skipUnless(MQTT_BROKER_AVAILABLE, "SimpleMQTTBroker not available")
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

    @unittest.skipUnless(MQTT_BROKER_AVAILABLE, "SimpleMQTTBroker not available")
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


class TestDeploymentCLI(unittest.TestCase):
    """Test DeploymentCLI functionality"""

    def setUp(self):
        """Set up test fixtures"""
        if DEPLOYMENT_CLI_AVAILABLE:
            self.temp_dir = tempfile.mkdtemp()
            self.cli = DeploymentCLI()
            self.cli.upload_dir = Path(self.temp_dir) / "server_dir"
            self.cli.upload_dir.mkdir(exist_ok=True)

    def tearDown(self):
        """Clean up test fixtures"""
        if hasattr(self, 'temp_dir'):
            import shutil
            shutil.rmtree(self.temp_dir, ignore_errors=True)

    @unittest.skipUnless(DEPLOYMENT_CLI_AVAILABLE, "DeploymentCLI not available")
    def test_cli_initialization(self):
        """Test CLI initialization"""
        cli = DeploymentCLI()
        self.assertIsNotNone(cli.logger)
        self.assertIsNone(cli.mqtt_broker)
        self.assertEqual(cli.deployed_ai_models, [])
        self.assertEqual(cli.deployed_modules, [])
        self.assertEqual(cli.http_port, 8000)

    @unittest.skipUnless(DEPLOYMENT_CLI_AVAILABLE, "DeploymentCLI not available")
    def test_cli_initialization_with_custom_host(self):
        """Test CLI initialization with custom HTTP host"""
        custom_host = "192.168.1.100"
        cli = DeploymentCLI(http_host=custom_host)
        self.assertEqual(cli.http_host, custom_host)

    @unittest.skipUnless(DEPLOYMENT_CLI_AVAILABLE, "DeploymentCLI not available")
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

    @unittest.skipUnless(DEPLOYMENT_CLI_AVAILABLE, "DeploymentCLI not available")
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

    @unittest.skipUnless(DEPLOYMENT_CLI_AVAILABLE, "DeploymentCLI not available")
    def test_copy_nonexistent_file(self):
        """Test copying non-existent file raises error"""
        nonexistent_file = Path(self.temp_dir) / "nonexistent.txt"

        with self.assertRaises(FileNotFoundError):
            self.cli.copy_file_to_upload_dir(nonexistent_file)

    @unittest.skipUnless(DEPLOYMENT_CLI_AVAILABLE, "DeploymentCLI not available")
    def test_list_deployed_empty(self):
        """Test listing deployed items when none exist"""
        with patch.object(self.cli.logger, 'info') as mock_logger:
            self.cli.list_deployed()

            # Check that it logs about no deployed items
            calls = mock_logger.call_args_list
            self.assertTrue(any("None" in str(call) for call in calls))


class TestMQTTMessageHandling(unittest.TestCase):
    """Test MQTT message handling and callback functionality"""

    def setUp(self):
        """Set up test fixtures"""
        if MQTT_BROKER_AVAILABLE:
            self.broker = SimpleMQTTBroker()

    @unittest.skipUnless(MQTT_BROKER_AVAILABLE, "SimpleMQTTBroker not available")
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


class TestDeploymentFunctionality(unittest.TestCase):
    """Test deployment functionality with mocked dependencies"""

    def setUp(self):
        """Set up test fixtures"""
        if DEPLOYMENT_CLI_AVAILABLE:
            self.temp_dir = tempfile.mkdtemp()
            self.cli = DeploymentCLI()
            self.cli.upload_dir = Path(self.temp_dir) / "server_dir"
            self.cli.upload_dir.mkdir(exist_ok=True)

            # Mock MQTT broker
            self.mock_broker = Mock()
            self.cli.mqtt_broker = self.mock_broker

    def tearDown(self):
        """Clean up test fixtures"""
        if hasattr(self, 'temp_dir'):
            import shutil
            shutil.rmtree(self.temp_dir, ignore_errors=True)

    @unittest.skipUnless(DEPLOYMENT_CLI_AVAILABLE, "DeploymentCLI not available")
    def test_deploy_ai_model_file_not_found(self):
        """Test AI model deployment with non-existent file"""
        result = self.cli.deploy_ai_model("nonexistent_model.bin")
        self.assertFalse(result)
        self.assertEqual(len(self.cli.deployed_ai_models), 0)

    @unittest.skipUnless(DEPLOYMENT_CLI_AVAILABLE, "DeploymentCLI not available")
    def test_deploy_wasm_file_not_found(self):
        """Test WASM deployment with non-existent file"""
        result = self.cli.deploy_wasm("nonexistent_module.wasm")
        self.assertFalse(result)
        self.assertEqual(len(self.cli.deployed_modules), 0)


class TestErrorHandling(unittest.TestCase):
    """Test error handling and edge cases"""

    def setUp(self):
        """Set up test fixtures"""
        if MQTT_BROKER_AVAILABLE:
            self.broker = SimpleMQTTBroker()

    @unittest.skipUnless(MQTT_BROKER_AVAILABLE, "SimpleMQTTBroker not available")
    def test_mqtt_message_with_invalid_json(self):
        """Test handling of MQTT messages with invalid JSON"""
        mock_msg = Mock()
        mock_msg.topic = "test/topic"
        mock_msg.payload.decode.return_value = "invalid json {"

        # This should not raise an exception
        self.broker._on_message(None, None, mock_msg)

        # Message should still be stored with the invalid JSON
        self.assertEqual(len(self.broker.recent_messages), 1)

    @unittest.skipUnless(MQTT_BROKER_AVAILABLE, "SimpleMQTTBroker not available")
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

    @unittest.skipUnless(MQTT_BROKER_AVAILABLE, "SimpleMQTTBroker not available")
    def test_show_recent_messages_empty(self):
        """Test showing recent messages when none exist"""
        with patch.object(self.broker._logger, 'error') as mock_logger:
            self.broker.show_recent_messages()
            mock_logger.assert_called_with("No recent messages")


def print_test_summary():
    """Print a summary of what will be tested"""
    print("Deploy CLI Functional Test Suite")
    print("=" * 50)
    print(f"get_local_ip available: {GET_LOCAL_IP_AVAILABLE}")
    print(f"SimpleMQTTBroker available: {MQTT_BROKER_AVAILABLE}")
    print(f"DeploymentCLI available: {DEPLOYMENT_CLI_AVAILABLE}")
    print()

    if not any([GET_LOCAL_IP_AVAILABLE, MQTT_BROKER_AVAILABLE, DEPLOYMENT_CLI_AVAILABLE]):
        print("⚠️  Warning: No CLI components could be imported!")
        print("   This usually means missing dependencies.")
        print("   Check that paho-mqtt and other dependencies are installed.")
        print()

    available_tests = 0
    total_tests = 0

    for test_class in [TestGetLocalIP, TestSimpleMQTTBroker, TestDeploymentCLI,
                      TestMQTTMessageHandling, TestDeploymentFunctionality, TestErrorHandling]:
        class_tests = [method for method in dir(test_class) if method.startswith('test_')]
        total_tests += len(class_tests)

        for test_method in class_tests:
            method = getattr(test_class, test_method)
            if not hasattr(method, '__unittest_skip__'):
                available_tests += 1

    print(f"Tests available to run: {available_tests}/{total_tests}")
    print()


if __name__ == "__main__":
    print_test_summary()

    # Run the tests
    unittest.main(verbosity=2)
