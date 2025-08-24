# Local Test Tool Test Documentation

## Overview

This directory contains comprehensive test suites for the AI Model and WASM Deployment CLI Tool. The tests are designed to ensure reliability, error handling, and proper functionality of all CLI features.

## Test Structure

```
tools/edgeapp_cli/tests/
├── test_basic.py               # Basic functionality tests (no complex imports)
├── test_deploy_cli.py          # Main unit tests for deployment CLI
├── test_configuration_sending.py # Configuration sending and MQTT tests
├── test_functional.py          # Functional tests with proper import handling
├── test_integration.py         # Integration tests with real services
├── test_helpers.py             # Test utilities and helper classes
├── run_tests.py               # Test runner script
├── test_requirements.txt       # Test dependencies
├── pytest.ini                # Pytest configuration
├── __init__.py                # Python package initialization
└── TEST_README.md             # This file
```

## Test Categories

### 1. Basic Tests (test_basic.py)
- **TestBasicFunctionality**: Core Python environment and file operations
- **TestMockingCapabilities**: Mock object functionality verification
- **TestHashCalculation**: File hash computation (SHA256)
- **TestNetworkingMocks**: Network operation mocking
- **TestJSONHandling**: JSON parsing and validation
- **TestThreadingBasics**: Thread operations and management

### 2. Unit Tests (test_deploy_cli.py)
- **TestGetLocalIP**: IP address detection functionality
- **TestSimpleMQTTBroker**: MQTT broker wrapper functionality
- **TestDeploymentCLI**: CLI initialization and basic operations
- **TestDeploymentFunctionality**: Deployment workflows with mocked dependencies
- **TestMQTTMessageHandling**: MQTT message processing and callbacks
- **TestErrorHandling**: Error handling and edge cases

### 3. Configuration Tests (test_configuration_sending.py)
- **TestConfigurationSending**: Configuration file parsing and MQTT sending
- **TestSimpleMQTTBroker**: MQTT broker configuration and message handling

### 4. Functional Tests (test_functional.py)
- **TestGetLocalIP**: IP detection with proper import handling
- **TestSimpleMQTTBroker**: MQTT functionality with error recovery
- **TestDeploymentCLI**: CLI operations with import error handling
- **TestMQTTMessageHandling**: Advanced MQTT message processing
- **TestDeploymentFunctionality**: End-to-end deployment scenarios
- **TestErrorHandling**: Comprehensive error handling and recovery

### 5. Integration Tests (test_integration.py)
- **TestMQTTIntegration**: Real MQTT broker connectivity tests
- **TestHTTPServerIntegration**: HTTP server functionality tests
- **TestFullDeploymentFlow**: End-to-end deployment workflows
- **TestErrorRecovery**: Error recovery and resilience tests

### 6. Test Helpers (test_helpers.py)
- **TestFileHelper**: Utilities for creating test files and directories
- **TestScenarios**: Pre-built test scenarios and data structures

## Running Tests

### Method 1: Using the Test Runner (Recommended)
```bash
# Run all tests automatically (tries pytest, falls back to unittest)
python run_tests.py

# Run with specific test framework
python run_tests.py unittest
python run_tests.py pytest

# Run with coverage report
python run_tests.py coverage
```

### Method 2: Using unittest directly
```bash
# Run all unit tests
python -m unittest test_deploy_cli.py -v

# Run basic functionality tests (no complex imports required)
python -m unittest test_basic.py -v

# Run configuration sending tests
python -m unittest test_configuration_sending.py -v

# Run functional tests
python -m unittest test_functional.py -v

# Run integration tests
python -m unittest test_integration.py -v

# Run specific test class
python -m unittest test_deploy_cli.TestDeploymentCLI -v

# Run specific test method
python -m unittest test_deploy_cli.TestDeploymentCLI.test_cli_initialization -v
```

### Method 3: Using pytest (if installed)
```bash
# Install test dependencies first
pip install -r test_requirements.txt

# Run all tests
pytest -v

# Run specific test file
pytest test_deploy_cli.py -v

# Run with coverage
pytest --cov=deploy_cli --cov-report=html

# Run only basic tests (no external dependencies)
pytest test_basic.py -v

# Run only integration tests
pytest test_integration.py -v

# Run tests with specific markers
pytest -m "not slow" -v
```

## Test Configuration

### Environment Variables for Integration Tests
```bash
# Optional: Set MQTT broker for real connectivity tests
export MQTT_TEST_HOST=localhost
export MQTT_TEST_PORT=1883

# Run integration tests
python -m pytest test_integration.py -m integration
```

### Pytest Configuration (pytest.ini)
- Test discovery patterns
- Coverage reporting
- HTML coverage output
- Test markers for categorization

## Test Dependencies

Install test dependencies:
```bash
pip install -r test_requirements.txt
```

Required packages:
- `pytest>=7.0.0` - Modern test framework
- `pytest-cov>=4.0.0` - Coverage reporting
- `pytest-mock>=3.10.0` - Advanced mocking
- `coverage>=7.0.0` - Code coverage analysis

## Test Coverage

The test suite aims for comprehensive coverage of:

### Core Functionality
- CLI initialization and configuration
- MQTT broker connection and messaging
- HTTP server startup and file serving
- AI model deployment workflow
- WASM module deployment workflow
- Configuration file parsing and sending
- File copying and URL generation
- Hash calculation and verification (SHA256 for WASM, Base64 for AI models)
- Device initialization and telemetry detection

### Error Handling
- Network connection failures
- File not found errors
- MQTT publish failures
- Deployment timeouts
- Invalid input handling
- Callback exception handling
- Import error handling (graceful degradation)
- JSON parsing errors
- Port conflicts and permission errors

### Edge Cases 
- Empty deployment lists
- Invalid JSON messages
- Port conflicts
- Permission errors
- Keyboard interrupt handling
- Mock object JSON serialization
- HTTP Range request handling
- Configuration endpoint management

## Test Data and Fixtures

### Temporary Files
Tests create temporary files and directories that are automatically cleaned up:
- Fake AI model files (.bin)
- Fake WASM module files (.wasm)
- Upload directories (server_dir/)
- Configuration files (.json)
- Server logs (server_logs/)

### Mock Objects
- **MockMQTTBroker**: Simulates MQTT broker responses and message handling
- **MockHTTPServer**: Simulates HTTP server behavior with Range request support
- **Mock OnWireSchema**: Provides serializable deployment message structures
- **TestFileHelper**: Creates and manages test files with proper cleanup

### Test Utilities
- **Import Error Handling**: Graceful degradation when dependencies are missing
- **Conditional Test Execution**: Tests skip automatically if dependencies unavailable
- **Server Directory Management**: Automatic cleanup of test server directories
- **Log Capture**: Verification of log messages and error conditions

## Debugging Failed Tests

### Common Issues and Solutions

1. **Import Errors**
   ```bash
   # Ensure you're in the correct directory
   cd tools/edgeapp_cli/tests
   
   # Run basic tests first (no complex imports)
   python -m unittest test_basic.py -v
   
   # Check Python path
   python -c "import sys; print(sys.path)"
   ```

2. **Module Import Failures**
   ```bash
   # Tests are designed to handle import failures gracefully
   # Check if dependencies are available
   python -c "from deploy_cli import DeploymentCLI"
   
   # Run only tests that don't require complex imports
   python -m unittest test_basic.py test_helpers.py -v
   ```

3. **MQTT Connection Failures**
   ```bash
   # Check if MQTT broker is running (for integration tests)
   netstat -an | grep 1883
   
   # Skip MQTT tests if broker not available
   pytest -k "not mqtt" -v
   
   # Run configuration tests with mocked MQTT
   python -m unittest test_configuration_sending.py -v
   ```

4. **Port Conflicts**
   ```bash
   # Check for port usage
   netstat -tulpn | grep 8000
   
   # Kill processes using test ports
   sudo fuser -k 8000/tcp
   
   # Tests should handle port conflicts gracefully
   ```

5. **Permission Errors**
   ```bash
   # Ensure write permissions for temp directories
   chmod 755 /tmp
   
   # Clean up test server directories
   rm -rf tools/edgeapp_cli/tests/server_dir/*
   
   # Run tests with proper permissions
   python run_tests.py
   ```

6. **JSON Serialization Errors**
   ```bash
   # Tests include fixes for Mock object serialization
   # Check if OnWireSchema mocks are properly configured
   python -m unittest test_deploy_cli.TestDeploymentFunctionality.test_deploy_wasm_success -v
   ```

### Verbose Output
```bash
# Run with maximum verbosity using unittest
python -m unittest test_deploy_cli.py -v

# Run basic tests with verbose output
python -m unittest test_basic.py -v

# Pytest with detailed output
pytest test_deploy_cli.py -v -s --tb=long

# Run all tests with coverage and verbose output
python run_tests.py coverage
```

## Test Organization and Import Handling

### Graceful Import Handling
The test suite is designed to handle missing dependencies gracefully:

```python
# Tests check for import availability
try:
    from deploy_cli import DeploymentCLI
    DEPLOY_CLI_AVAILABLE = True
except ImportError:
    DEPLOY_CLI_AVAILABLE = False

# Tests are conditionally skipped
@unittest.skipUnless(DEPLOY_CLI_AVAILABLE, "deploy_cli module not available")
class TestDeploymentCLI(unittest.TestCase):
    # Test implementation
```

### Test Execution Order
1. **test_basic.py** - Run first (no complex dependencies)
2. **test_helpers.py** - Test utilities and helpers
3. **test_functional.py** - Functional tests with import error handling
4. **test_deploy_cli.py** - Main unit tests
5. **test_configuration_sending.py** - Configuration and MQTT tests
6. **test_integration.py** - Integration tests (require external services)

## Adding New Tests

### Test Naming Convention
- Test files: `test_*.py`
- Test classes: `Test*`
- Test methods: `test_*`

### Example Test Structure
```python
class TestNewFeature(unittest.TestCase):
    def setUp(self):
        """Set up test fixtures"""
        self.cli = DeploymentCLI()
        
    def tearDown(self):
        """Clean up after tests"""
        pass
        
    def test_new_functionality(self):
        """Test description"""
        # Arrange
        expected_result = True
        
        # Act
        result = self.cli.new_method()
        
        # Assert
        self.assertEqual(result, expected_result)
```

### Integration Test Markers
```python
@pytest.mark.integration
@pytest.mark.slow
def test_slow_integration_feature(self):
    """Test that requires real services"""
    pass
```
