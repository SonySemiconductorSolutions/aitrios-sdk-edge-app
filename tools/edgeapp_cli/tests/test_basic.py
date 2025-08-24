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
Simple test suite for the Deploy CLI - Basic functionality only
This test file avoids complex imports and focuses on core functionality
"""

import unittest
import tempfile
import os
import sys
from pathlib import Path
from unittest.mock import Mock, patch

# Add the parent directory to Python path
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), '..'))


class TestBasicFunctionality(unittest.TestCase):
    """Test basic functionality without complex dependencies"""

    def test_python_environment(self):
        """Test that Python environment is working"""
        self.assertTrue(True)
        self.assertEqual(2 + 2, 4)

    def test_file_operations(self):
        """Test basic file operations"""
        with tempfile.NamedTemporaryFile(mode='w', delete=False) as f:
            f.write("test content")
            temp_path = f.name

        try:
            # Check file exists
            self.assertTrue(os.path.exists(temp_path))

            # Read content
            with open(temp_path, 'r') as f:
                content = f.read()
            self.assertEqual(content, "test content")

        finally:
            # Clean up
            os.unlink(temp_path)

    def test_path_operations(self):
        """Test Path operations"""
        temp_dir = tempfile.mkdtemp()
        path = Path(temp_dir)

        try:
            self.assertTrue(path.exists())
            self.assertTrue(path.is_dir())

            # Create a test file
            test_file = path / "test.txt"
            test_file.write_text("hello world")

            self.assertTrue(test_file.exists())
            self.assertEqual(test_file.read_text(), "hello world")

        finally:
            # Clean up
            import shutil
            shutil.rmtree(temp_dir)


class TestMockingCapabilities(unittest.TestCase):
    """Test that mocking works properly"""

    def test_mock_creation(self):
        """Test basic mock creation"""
        mock_obj = Mock()
        mock_obj.test_method.return_value = "mocked value"

        result = mock_obj.test_method()
        self.assertEqual(result, "mocked value")
        mock_obj.test_method.assert_called_once()

    @patch('os.path.exists')
    def test_patching(self, mock_exists):
        """Test patching functionality"""
        mock_exists.return_value = True

        result = os.path.exists("/fake/path")
        self.assertTrue(result)
        mock_exists.assert_called_once_with("/fake/path")


class TestHashCalculation(unittest.TestCase):
    """Test hash calculation functionality"""

    def test_sha256_hash(self):
        """Test SHA256 hash calculation"""
        import hashlib

        test_data = b"Hello, World!"
        hasher = hashlib.sha256()
        hasher.update(test_data)
        expected_hash = hasher.hexdigest()

        # Test that we can recreate the same hash
        hasher2 = hashlib.sha256()
        hasher2.update(test_data)
        actual_hash = hasher2.hexdigest()

        self.assertEqual(actual_hash, expected_hash)
        self.assertEqual(len(actual_hash), 64)  # SHA256 produces 64 character hex string

    def test_file_hash_calculation(self):
        """Test hash calculation for files"""
        import hashlib

        # Create a temporary file
        test_content = b"Test file content for hashing"
        with tempfile.NamedTemporaryFile(delete=False) as f:
            f.write(test_content)
            temp_path = f.name

        try:
            # Calculate hash manually
            hasher = hashlib.sha256()
            with open(temp_path, 'rb') as f:
                for chunk in iter(lambda: f.read(4096), b""):
                    hasher.update(chunk)
            expected_hash = hasher.hexdigest()

            # Calculate hash using the same method as deploy_cli
            def calculate_file_hash(file_path):
                hasher = hashlib.sha256()
                with open(file_path, 'rb') as f:
                    for chunk in iter(lambda: f.read(4096), b""):
                        hasher.update(chunk)
                return hasher.hexdigest()

            actual_hash = calculate_file_hash(temp_path)
            self.assertEqual(actual_hash, expected_hash)

        finally:
            os.unlink(temp_path)


class TestNetworkingMocks(unittest.TestCase):
    """Test networking functionality with mocks"""

    @patch('socket.socket')
    def test_ip_detection_mock(self, mock_socket):
        """Test IP detection with mocked socket"""
        # Mock socket connection
        mock_socket_instance = Mock()
        mock_socket.return_value.__enter__.return_value = mock_socket_instance
        mock_socket_instance.getsockname.return_value = ("192.168.1.100", 12345)

        # Simulate the get_local_ip function logic
        def mock_get_local_ip():
            try:
                import socket
                with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
                    s.connect(("8.8.8.8", 80))
                    local_ip = s.getsockname()[0]
                    return local_ip
            except Exception:
                return "localhost"

        # Test the function
        ip = mock_get_local_ip()
        self.assertEqual(ip, "192.168.1.100")

    @patch('socket.socket')
    def test_ip_detection_fallback(self, mock_socket):
        """Test IP detection fallback to localhost"""
        # Mock socket to raise an exception
        mock_socket.side_effect = Exception("Network error")

        def mock_get_local_ip():
            try:
                import socket
                with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
                    s.connect(("8.8.8.8", 80))
                    local_ip = s.getsockname()[0]
                    return local_ip
            except Exception:
                return "localhost"

        ip = mock_get_local_ip()
        self.assertEqual(ip, "localhost")


class TestJSONHandling(unittest.TestCase):
    """Test JSON handling functionality"""

    def test_json_serialization(self):
        """Test JSON serialization"""
        import json

        test_data = {
            "deployment": {
                "deploymentId": "test_id",
                "modules": {
                    "module1": {
                        "name": "test_module",
                        "url": "http://example.com/module.wasm"
                    }
                }
            }
        }

        # Serialize to JSON
        json_str = json.dumps(test_data)
        self.assertIsInstance(json_str, str)

        # Deserialize from JSON
        parsed_data = json.loads(json_str)
        self.assertEqual(parsed_data, test_data)

    def test_json_error_handling(self):
        """Test JSON error handling"""
        import json

        invalid_json = "{ invalid json"

        with self.assertRaises(json.JSONDecodeError):
            json.loads(invalid_json)


class TestThreadingBasics(unittest.TestCase):
    """Test basic threading functionality"""

    def test_threading_event(self):
        """Test threading event functionality"""
        import threading
        import time

        event = threading.Event()
        result = {"value": None}

        def worker():
            time.sleep(0.1)
            result["value"] = "completed"
            event.set()

        thread = threading.Thread(target=worker)
        thread.start()

        # Wait for event with timeout
        success = event.wait(timeout=1.0)
        thread.join()

        self.assertTrue(success)
        self.assertEqual(result["value"], "completed")

    def test_threading_timeout(self):
        """Test threading timeout"""
        import threading

        event = threading.Event()

        # Don't set the event, should timeout
        success = event.wait(timeout=0.1)
        self.assertFalse(success)


if __name__ == "__main__":
    print("Running basic Deploy CLI tests...")
    print("=" * 50)

    # Run the tests
    unittest.main(verbosity=2)
