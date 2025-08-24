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
Integration tests for the AI Model and WASM Deployment CLI Tool
These tests require actual MQTT broker and HTTP server functionality
"""

import json
import os
import tempfile
import threading
import time
import unittest
from pathlib import Path
from unittest.mock import Mock, patch
from uuid import uuid4

# pytest markers for conditional testing

# Use unittest.skipIf instead of pytest markers when pytest is not available
try:
    import pytest
    PYTEST_AVAILABLE = True
except ImportError:
    PYTEST_AVAILABLE = False
    # Create dummy decorators for unittest compatibility
    class pytest:
        class mark:
            @staticmethod
            def integration(func):
                return func
            @staticmethod
            def slow(func):
                return func
            @staticmethod
            def skipif(condition, reason=""):
                import unittest
                return unittest.skipIf(condition, reason)
        @staticmethod
        def skip(reason):
            import unittest
            raise unittest.SkipTest(reason)
        @staticmethod
        def main(args):
            import unittest
            unittest.main(verbosity=2)

from edgeapp_cli import DeploymentCLI, SimpleMQTTBroker


@pytest.mark.integration
class TestMQTTIntegration(unittest.TestCase):
    """Integration tests with real MQTT broker (if available)"""

    def setUp(self):
        """Set up test fixtures"""
        self.broker = SimpleMQTTBroker()
        self.mqtt_available = False

        # Try to connect to local MQTT broker
        try:
            if self.broker.connect():
                self.mqtt_available = True
                time.sleep(1)  # Allow connection to stabilize
        except Exception:
            pass

    def tearDown(self):
        """Clean up test fixtures"""
        if self.mqtt_available and self.broker:
            self.broker.disconnect()

    @pytest.mark.skipif(not os.environ.get('MQTT_TEST_HOST'),
                       reason="MQTT_TEST_HOST environment variable not set")
    def test_real_mqtt_connection(self):
        """Test connection to real MQTT broker"""
        mqtt_host = os.environ.get('MQTT_TEST_HOST', 'localhost')
        mqtt_port = int(os.environ.get('MQTT_TEST_PORT', '1883'))

        broker = SimpleMQTTBroker(mqtt_host, mqtt_port)
        success = broker.connect()

        if success:
            self.assertTrue(broker.connected)
            broker.disconnect()
            self.assertFalse(broker.connected)
        else:
            pytest.skip(f"Could not connect to MQTT broker at {mqtt_host}:{mqtt_port}")

    def test_mqtt_message_publishing(self):
        """Test MQTT message publishing functionality"""
        if not self.mqtt_available:
            pytest.skip("MQTT broker not available")

        # Test publishing a simple message
        topic = "test/deployment"
        message = {"test": "data", "timestamp": time.time()}

        result = self.broker.client.publish(topic, json.dumps(message))
        self.assertEqual(result.rc, 0)  # Success

    def test_mqtt_attribute_subscription(self):
        """Test MQTT attribute subscription and message reception"""
        if not self.mqtt_available:
            pytest.skip("MQTT broker not available")

        # Wait a bit to ensure subscription is active
        time.sleep(1)

        # Publish a test attribute message
        test_data = {"testAttribute": "testValue"}
        result = self.broker.client.publish("v1/devices/me/attributes", json.dumps(test_data))
        self.assertEqual(result.rc, 0)

        # Wait for message to be received
        time.sleep(2)

        # Check if message was received and stored
        self.assertIn("testAttribute", self.broker.received_attributes)
        self.assertEqual(self.broker.received_attributes["testAttribute"], "testValue")


@pytest.mark.integration
class TestHTTPServerIntegration(unittest.TestCase):
    """Integration tests for HTTP server functionality"""

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
        if hasattr(self.cli, 'http_server_thread') and self.cli.http_server_thread:
            # Clean up HTTP server if running
            pass

    @pytest.mark.slow
    def test_http_server_startup_and_file_serving(self):
        """Test HTTP server startup and file serving capability"""
        # Create a test file
        test_file = self.cli.upload_dir / "test.txt"
        test_content = "Hello, World!"
        test_file.write_text(test_content)

        # Start HTTP server
        success = self.cli.start_http_server(port=8001)  # Use different port to avoid conflicts

        if success:
            # Test file accessibility
            import urllib.request
            import urllib.error

            try:
                url = f"http://{self.cli.http_host}:8001/test.txt"
                response = urllib.request.urlopen(url, timeout=5)
                content = response.read().decode()
                self.assertEqual(content, test_content)
            except urllib.error.URLError:
                pytest.skip("HTTP server not accessible (may be firewall or network issue)")
        else:
            pytest.skip("HTTP server failed to start")


@pytest.mark.integration
class TestFullDeploymentFlow(unittest.TestCase):
    """Integration tests for full deployment workflow"""

    def setUp(self):
        """Set up test fixtures"""
        self.temp_dir = tempfile.mkdtemp()
        self.cli = DeploymentCLI()
        self.cli.upload_dir = Path(self.temp_dir) / "server_dir"
        self.cli.upload_dir.mkdir(exist_ok=True)

        # Mock MQTT broker for integration testing
        self.mock_broker = Mock()
        self.cli.mqtt_broker = self.mock_broker

    def tearDown(self):
        """Clean up test fixtures"""
        import shutil
        shutil.rmtree(self.temp_dir, ignore_errors=True)

    def test_end_to_end_ai_model_deployment(self):
        """Test complete AI model deployment workflow"""
        # Create a fake AI model file
        model_file = Path(self.temp_dir) / "test_model.bin"
        model_data = b"Fake AI model binary data" * 1000  # Make it reasonably sized
        model_file.write_bytes(model_data)

        # Configure mock responses
        self.mock_broker.client.publish.return_value.rc = 0
        self.mock_broker.wait_for_ai_model_config_response.return_value = (True, None)
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

        # Mock HTTP server as running
        self.cli.http_server_running = True

        with patch('webserver.CustomHTTPRequestHandler.set_log_suppression'):
            # Execute deployment
            result = self.cli.deploy_ai_model(str(model_file), "integration_test_model")

        # Verify deployment succeeded
        self.assertTrue(result)
        self.assertEqual(len(self.cli.deployed_ai_models), 1)

        deployed_model = self.cli.deployed_ai_models[0]
        self.assertEqual(deployed_model.version, "integration_test_model")
        self.assertEqual(deployed_model.name, "test_model")

        # Verify file was copied to upload directory
        uploaded_file = self.cli.upload_dir / "test_model.bin"
        self.assertTrue(uploaded_file.exists())
        self.assertEqual(uploaded_file.read_bytes(), model_data)

    def test_end_to_end_wasm_deployment(self):
        """Test complete WASM module deployment workflow"""
        # Create a fake WASM file
        wasm_file = Path(self.temp_dir) / "test_module.wasm"
        wasm_data = b"Fake WASM module binary data" * 500
        wasm_file.write_bytes(wasm_data)

        # Configure mock responses
        self.mock_broker.client.publish.return_value.rc = 0
        self.mock_broker.wait_for_deployment_status.return_value = (True, None)
        self.mock_broker.verbose_logging = False

        # Mock HTTP server as running
        self.cli.http_server_running = True

        # Mock the onwire_schema.to_deployment to return a serializable dict
        self.mock_broker.onwire_schema.to_deployment.return_value = {
            "reqId": "test_req_id",
            "instance": "$system",
            "topic": "PRIVATE_deploy_wasm",
            "deploy": {
                "targets": [{"instance": "integration_instance_id", "hash": "test_hash"}],
                "req_info": {"req_id": "test_req_id"}
            }
        }

        with patch('webserver.CustomHTTPRequestHandler.set_log_suppression'), \
             patch.object(self.cli, '_get_deployed_wasm_instance_by_hash', return_value="integration_instance_id"):
            # Execute deployment
            result = self.cli.deploy_wasm(str(wasm_file), "integration_test_module")

        # Verify deployment succeeded
        self.assertTrue(result)
        self.assertEqual(len(self.cli.deployed_modules), 1)

        deployed_module = self.cli.deployed_modules[0]
        self.assertEqual(deployed_module.name, "integration_test_module")

        # Verify file was copied to upload directory
        uploaded_file = self.cli.upload_dir / "test_module.wasm"
        self.assertTrue(uploaded_file.exists())
        self.assertEqual(uploaded_file.read_bytes(), wasm_data)

    def test_deployment_with_generated_ids(self):
        """Test deployment with auto-generated IDs"""
        # Create test files
        model_file = Path(self.temp_dir) / "auto_id_model.bin"
        model_file.write_bytes(b"test model data")

        wasm_file = Path(self.temp_dir) / "auto_id_module.wasm"
        wasm_file.write_bytes(b"test wasm data")

        # Configure mock responses
        self.mock_broker.client.publish.return_value.rc = 0
        self.mock_broker.wait_for_ai_model_config_response.return_value = (True, None)
        self.mock_broker.wait_for_deployment_status.return_value = (True, None)
        self.mock_broker.verbose_logging = False
        self.cli.http_server_running = True

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

        # Mock the onwire_schema.to_deployment to return a serializable dict
        self.mock_broker.onwire_schema.to_deployment.return_value = {
            "reqId": "test_req_id",
            "instance": "$system",
            "topic": "PRIVATE_deploy_wasm",
            "deploy": {
                "targets": [{"instance": "auto_instance_id", "hash": "test_hash"}],
                "req_info": {"req_id": "test_req_id"}
            }
        }

        with patch('webserver.CustomHTTPRequestHandler.set_log_suppression'), \
             patch.object(self.cli, '_get_deployed_wasm_instance_by_hash', return_value="auto_instance_id"):
            # Deploy without specifying IDs
            ai_result = self.cli.deploy_ai_model(str(model_file))  # No model_id
            wasm_result = self.cli.deploy_wasm(str(wasm_file))     # No module_name

        # Verify both deployments succeeded
        self.assertTrue(ai_result)
        self.assertTrue(wasm_result)

        # Verify IDs were auto-generated
        self.assertEqual(len(self.cli.deployed_ai_models), 1)
        self.assertEqual(len(self.cli.deployed_modules), 1)

        ai_model = self.cli.deployed_ai_models[0]
        wasm_module = self.cli.deployed_modules[0]

        # Check that version was set to default
        self.assertEqual(ai_model.version, "0311030000000123")  # Default version
        self.assertEqual(wasm_module.name, "auto_id_module")  # Uses file stem


@pytest.mark.integration
class TestErrorRecovery(unittest.TestCase):
    """Integration tests for error handling and recovery"""

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

    def test_deployment_timeout_recovery(self):
        """Test recovery from deployment timeout"""
        # Create test file
        model_file = Path(self.temp_dir) / "timeout_model.bin"
        model_file.write_bytes(b"test data")

        # Configure mock to simulate timeout
        self.mock_broker.client.publish.return_value.rc = 0
        self.mock_broker.wait_for_ai_model_config_response.return_value = (False, "Timeout after 120 seconds")
        self.mock_broker.verbose_logging = False
        self.cli.http_server_running = True

        with patch('webserver.CustomHTTPRequestHandler.set_log_suppression'):
            result = self.cli.deploy_ai_model(str(model_file))

        # Verify deployment failed gracefully
        self.assertFalse(result)
        self.assertEqual(len(self.cli.deployed_ai_models), 0)

        # CLI should still be operational for next deployment
        self.assertIsNotNone(self.cli.mqtt_broker)

    def test_network_error_recovery(self):
        """Test recovery from network errors"""
        # Create test file
        model_file = Path(self.temp_dir) / "network_error_model.bin"
        model_file.write_bytes(b"test data")

        # Configure mock to simulate network error
        self.mock_broker.client.publish.return_value.rc = 1  # Network error
        self.mock_broker.verbose_logging = False
        self.cli.http_server_running = True

        with patch('webserver.CustomHTTPRequestHandler.set_log_suppression'):
            result = self.cli.deploy_ai_model(str(model_file))

        # Verify deployment failed gracefully
        self.assertFalse(result)
        self.assertEqual(len(self.cli.deployed_ai_models), 0)


if __name__ == "__main__":
    # Run integration tests
    pytest.main([__file__, "-v", "-m", "integration"])
