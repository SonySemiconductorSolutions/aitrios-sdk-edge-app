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
Tests for configuration sending functionality in edgeapp_cli.py
"""

import json
import os
import tempfile
import unittest
from pathlib import Path
from unittest.mock import MagicMock, Mock, patch
import time
import uuid

import sys

# Add parent directory to path for imports
current_dir = Path(__file__).parent
parent_dir = current_dir.parent
sys.path.insert(0, str(parent_dir))

try:
    from edgeapp_cli import DeploymentCLI, SimpleMQTTBroker
    DEPLOY_CLI_AVAILABLE = True
except ImportError as e:
    print(f"Warning: Cannot import deploy_cli: {e}")
    DEPLOY_CLI_AVAILABLE = False
    # Create mock classes to allow test discovery
    class DeploymentCLI:
        pass
    class SimpleMQTTBroker:
        pass


@unittest.skipUnless(DEPLOY_CLI_AVAILABLE, "deploy_cli module not available")
class TestConfigurationSending(unittest.TestCase):
    """Test cases for configuration sending functionality"""

    def setUp(self):
        """Set up test fixtures"""
        self.cli = DeploymentCLI(http_host="192.168.1.100")
        self.cli.http_port = 8000
        self.cli.http_server_running = True

        # Mock the MQTT broker
        self.cli.mqtt_broker = Mock(spec=SimpleMQTTBroker)
        self.cli.mqtt_broker.connected = True
        self.cli.mqtt_broker.verbose_logging = False
        self.cli.mqtt_broker.broker_log_on = Mock()
        self.cli.mqtt_broker.broker_log_off = Mock()
        self.cli.mqtt_broker.send_configuration = Mock(return_value=(True, None))

        # Initialize received_attributes as a dictionary
        self.cli.mqtt_broker.received_attributes = {}

        # Mock deployment status for instance detection
        self.cli.mqtt_broker.received_attributes["deploymentStatus"] = {
            "deploymentId": "test-deployment-123",
            "reconcileStatus": "ok",
            "instances": {
                "test-instance-id-123": {
                    "status": "ok",
                    "moduleId": "test-module-456"
                }
            },
            "modules": {
                "test-module-456": {
                    "status": "ok"
                }
            }
        }

        # Sample configuration data
        self.sample_config = {
            "edge_app": {
                "req_info": {
                    "req_id": "test_req"
                },
                "common_settings": {
                    "process_state": 2,
                    "log_level": 5,
                    "port_settings": {
                        "metadata": {
                            "method": 2,
                            "storage_name": "",
                            "endpoint": "http://192.168.1.14:8000",
                            "path": "OT",
                            "enabled": True
                        },
                        "input_tensor": {
                            "method": 2,
                            "storage_name": "",
                            "endpoint": "http://192.168.1.14:8000",
                            "path": "IT",
                            "enabled": True
                        }
                    }
                },
                "custom_settings": {
                    "metadata_settings": {
                        "format": 1
                    }
                }
            }
        }

    def test_send_configuration_with_file(self):
        """Test sending configuration from a JSON file"""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            json.dump(self.sample_config, f, indent=2)
            config_file = f.name

        try:
            # Test successful configuration sending with instance auto-detection
            result = self.cli.send_configuration(config_file, "auto")

            self.assertTrue(result)
            self.cli.mqtt_broker.send_configuration.assert_called_once()

            # Verify the configuration was updated with new endpoints
            call_args = self.cli.mqtt_broker.send_configuration.call_args
            sent_config = call_args[0][0]  # First argument
            instance = call_args[0][1]      # Second argument

            # Should auto-detect the test instance
            self.assertEqual(instance, "test-instance-id-123")

            # Check that endpoints were updated
            metadata_endpoint = sent_config["edge_app"]["common_settings"]["port_settings"]["metadata"]["endpoint"]
            self.assertEqual(metadata_endpoint, "http://192.168.1.14:8000")

        finally:
            os.unlink(config_file)

    def test_send_configuration_with_process_state(self):
        """Test sending configuration with process_state option"""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            json.dump(self.sample_config, f, indent=2)
            config_file = f.name

        try:
            # Test with process_state=1
            result = self.cli.send_configuration(config_file, "$system", ps=1)

            self.assertTrue(result)

            # Verify process_state was updated
            call_args = self.cli.mqtt_broker.send_configuration.call_args
            sent_config = call_args[0][0]

            self.assertEqual(sent_config["edge_app"]["common_settings"]["process_state"], 1)

        finally:
            os.unlink(config_file)

    def test_send_configuration_with_instance(self):
        """Test sending configuration to specific instance"""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            json.dump(self.sample_config, f, indent=2)
            config_file = f.name

        try:
            # Test with specific instance
            result = self.cli.send_configuration(config_file, "my_edge_app_instance")

            self.assertTrue(result)

            # Verify instance parameter
            call_args = self.cli.mqtt_broker.send_configuration.call_args
            instance = call_args[0][1]

            self.assertEqual(instance, "my_edge_app_instance")

        finally:
            os.unlink(config_file)

    def test_send_configuration_file_not_found(self):
        """Test handling of missing configuration file"""
        result = self.cli.send_configuration("nonexistent_file.json")

        self.assertFalse(result)
        self.cli.mqtt_broker.send_configuration.assert_not_called()

    def test_send_configuration_invalid_json(self):
        """Test handling of invalid JSON configuration"""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            f.write("{ invalid json content }")
            config_file = f.name

        try:
            result = self.cli.send_configuration(config_file)

            self.assertFalse(result)
            self.cli.mqtt_broker.send_configuration.assert_not_called()

        finally:
            os.unlink(config_file)

    def test_send_configuration_mqtt_failure(self):
        """Test handling of MQTT sending failure"""
        # Configure MQTT broker to return failure
        self.cli.mqtt_broker.send_configuration.return_value = (False, "MQTT connection failed")

        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            json.dump(self.sample_config, f, indent=2)
            config_file = f.name

        try:
            result = self.cli.send_configuration(config_file)

            self.assertFalse(result)
            self.cli.mqtt_broker.send_configuration.assert_called_once()

        finally:
            os.unlink(config_file)

    def test_send_configuration_with_state_validation(self):
        """Test configuration sending with state message validation"""
        # Set up mock state messages
        self.cli.mqtt_broker.received_attributes = {
            "state/$system/edge_app": json.dumps({
                "res_info": {
                    "res_id": "test_req_123",
                    "code": 0,
                    "detail_msg": "Configuration applied successfully"
                },
                "process_state": 1
            })
        }

        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            json.dump(self.sample_config, f, indent=2)
            config_file = f.name

        try:
            with patch('time.sleep'):  # Mock sleep to speed up test
                result = self.cli.send_configuration(config_file)

            self.assertTrue(result)

        finally:
            os.unlink(config_file)

    def test_update_endpoints_no_http_server(self):
        """Test that endpoints are not updated when HTTP server is not running"""
        self.cli.http_server_running = False

        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            json.dump(self.sample_config, f, indent=2)
            config_file = f.name

        try:
            result = self.cli.send_configuration(config_file)

            self.assertTrue(result)

            # Verify endpoints were not updated
            call_args = self.cli.mqtt_broker.send_configuration.call_args
            sent_config = call_args[0][0]

            metadata_endpoint = sent_config["edge_app"]["common_settings"]["port_settings"]["metadata"]["endpoint"]
            self.assertEqual(metadata_endpoint, "http://192.168.1.14:8000")  # Original endpoint

        finally:
            os.unlink(config_file)

    def test_auto_instance_detection(self):
        """Test automatic WASM instance detection"""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            json.dump(self.sample_config, f, indent=2)
            config_file = f.name

        try:
            # Test with auto instance detection
            result = self.cli.send_configuration(config_file, "auto")

            self.assertTrue(result)

            # Verify that auto-detected instance was used
            call_args = self.cli.mqtt_broker.send_configuration.call_args
            instance = call_args[0][1]

            # Should have used the auto-detected instance
            self.assertEqual(instance, "test-instance-id-123")

        finally:
            os.unlink(config_file)

    def test_auto_req_id_generation(self):
        """Test automatic req_id generation"""
        config_data = {
            "edge_app": {
                "req_info": {
                    "req_id": "old_sample_id"
                },
                "common_settings": {
                    "metadata": {
                        "endpoint": "http://192.168.1.14:8000"
                    }
                }
            }
        }

        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            json.dump(config_data, f)
            config_file = f.name

        try:
            # Reset mock call count
            self.cli.mqtt_broker.send_configuration.reset_mock()

            # Mock time to ensure consistent testing
            with patch('time.time', return_value=1234567890):
                with patch('uuid.uuid4') as mock_uuid:
                    mock_uuid.return_value.hex = 'abcdef1234567890'
                    mock_uuid.return_value.__str__ = lambda: 'abcdef12-3456-7890-abcd-ef1234567890'

                    result = self.cli.send_configuration(config_file, "test-instance")

                    self.assertTrue(result)
                    self.cli.mqtt_broker.send_configuration.assert_called_once()

                    # Verify req_id was updated
                    call_args = self.cli.mqtt_broker.send_configuration.call_args
                    sent_config = call_args[0][0]  # First argument

                    req_id = sent_config["edge_app"]["req_info"]["req_id"]
                    self.assertTrue(req_id.startswith("cli1234567890"))
                    self.assertNotEqual(req_id, "old_sample_id")
        finally:
            os.unlink(config_file)

    def test_file_extension_auto_detection(self):
        """Test that .json extension is added automatically"""
        with tempfile.NamedTemporaryFile(mode='w', suffix='.json', delete=False) as f:
            json.dump(self.sample_config, f, indent=2)
            config_file = f.name

        try:
            # Remove .json extension from filename for test
            config_file_no_ext = config_file[:-5]  # Remove .json

            result = self.cli.send_configuration(config_file_no_ext)

            self.assertTrue(result)
            self.cli.mqtt_broker.send_configuration.assert_called_once()

        finally:
            os.unlink(config_file)


@unittest.skipUnless(DEPLOY_CLI_AVAILABLE, "deploy_cli module not available")
class TestSimpleMQTTBroker(unittest.TestCase):
    """Test cases for SimpleMQTTBroker configuration sending"""

    def setUp(self):
        """Set up test fixtures"""
        self.broker = SimpleMQTTBroker()
        self.broker.connected = True
        self.broker.client = Mock()
        self.broker.client.publish = Mock()
        self.broker.onwire_schema = Mock()
        self.broker.received_attributes = {}

    def test_send_configuration_success(self):
        """Test successful configuration sending"""
        # Mock OnWire schema
        mock_message = {
            "configuration/$system/edge_app": {
                "req_info": {"req_id": "test123"},
                "test_config": "test_value"
            }
        }
        self.broker.onwire_schema.to_config.return_value = mock_message

        # Mock successful MQTT publish
        mock_result = Mock()
        mock_result.rc = 0  # MQTT_ERR_SUCCESS
        self.broker.client.publish.return_value = mock_result

        # Test configuration payload
        config_payload = {
            "edge_app": {
                "test_config": "test_value"
            }
        }

        success, error = self.broker.send_configuration(config_payload, "$system")

        self.assertTrue(success)
        self.assertIsNone(error)

        # Verify MQTT publish was called
        self.broker.client.publish.assert_called_once()
        call_args = self.broker.client.publish.call_args
        self.assertEqual(call_args[0][0], "v1/devices/me/attributes")  # Topic

        # Verify OnWire schema was called correctly
        self.broker.onwire_schema.to_config.assert_called_once()

    def test_send_configuration_mqtt_failure(self):
        """Test configuration sending with MQTT failure"""
        # Mock MQTT publish failure
        mock_result = Mock()
        mock_result.rc = 1  # MQTT error
        self.broker.client.publish.return_value = mock_result

        # Mock OnWire schema
        self.broker.onwire_schema.to_config.return_value = {"test": "message"}

        config_payload = {"edge_app": {"test": "value"}}

        success, error = self.broker.send_configuration(config_payload, "$system")

        self.assertFalse(success)
        self.assertIsNotNone(error)
        self.assertIn("MQTT publish failed", error)

    def test_send_configuration_with_custom_topic(self):
        """Test configuration sending with custom topic"""
        # Mock OnWire schema
        self.broker.onwire_schema.to_config.return_value = {"test": "message"}

        # Mock successful MQTT publish
        mock_result = Mock()
        mock_result.rc = 0
        self.broker.client.publish.return_value = mock_result

        config_payload = {"custom_config": "value"}

        success, error = self.broker.send_configuration(config_payload, "$system", "custom_topic")

        self.assertTrue(success)

        # Verify OnWire schema was called with custom topic
        self.broker.onwire_schema.to_config.assert_called()
        call_args = self.broker.onwire_schema.to_config.call_args[1]  # kwargs
        self.assertEqual(call_args["topic"], "custom_topic")

    def test_send_configuration_auto_topic_detection(self):
        """Test automatic topic detection from payload"""
        # Mock OnWire schema
        self.broker.onwire_schema.to_config.return_value = {"test": "message"}

        # Mock successful MQTT publish
        mock_result = Mock()
        mock_result.rc = 0
        self.broker.client.publish.return_value = mock_result

        config_payload = {"edge_app": {"test": "value"}}

        success, error = self.broker.send_configuration(config_payload, "$system")

        self.assertTrue(success)

        # Verify OnWire schema was called with auto-detected topic
        self.broker.onwire_schema.to_config.assert_called()
        call_args = self.broker.onwire_schema.to_config.call_args[1]  # kwargs
        self.assertEqual(call_args["topic"], "edge_app")

    def test_configuration_structure_flattening(self):
        """Test that edge_app wrapper is properly removed for successful pattern matching"""
        # Mock OnWire schema
        self.broker.onwire_schema.to_config.return_value = {"test": "message"}

        # Mock successful MQTT publish
        mock_result = Mock()
        mock_result.rc = 0
        self.broker.client.publish.return_value = mock_result

        # Configuration with edge_app wrapper (CLI pattern)
        config_payload = {
            "edge_app": {
                "req_info": {"req_id": "test123"},
                "common_settings": {"process_state": 1},
                "custom_settings": {"ai_models": {"detection": {}}}
            }
        }

        success, error = self.broker.send_configuration(config_payload, "test-instance-123", "edge_app")

        self.assertTrue(success)

        # Verify OnWire schema was called
        self.broker.onwire_schema.to_config.assert_called()
        call_args = self.broker.onwire_schema.to_config.call_args[1]  # kwargs

        # Verify the config was flattened (edge_app wrapper removed)
        sent_config = call_args["config"]
        self.assertIn("req_info", sent_config)
        self.assertIn("common_settings", sent_config)
        self.assertIn("custom_settings", sent_config)
        # Should NOT have nested edge_app structure
        self.assertNotIn("edge_app", sent_config)


if __name__ == '__main__':
    # Run tests with verbose output
    unittest.main(verbosity=2)
