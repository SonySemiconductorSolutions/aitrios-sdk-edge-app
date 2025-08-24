# AI Model & Edge App Deployment CLI Tool

A simple CLI tool to deploy AI models and Edge App modules to the Edge Device.

## Features

- Automatic startup of HTTP server
- Easy deployment of AI models and Edge App files from command line
- Automatic file hash calculation
- Deployment history checking
- Comprehensive test suite (in `tests/` directory)

## Required Dependencies

It is recommended to use a Python virtual environment to manage dependencies.

```bash
# Create a virtual environment
python3 -m venv venv

# Activate the virtual environment
source venv/bin/activate
```

Then, install the required packages:

```bash
pip install -r requirements.txt
```
> **Note:**  
> This tool also depends on [aitrios/aitrios-edge-device-core](https://github.com/aitrios/aitrios-edge-device-core).   
Please ensure you have set up and configured the edge device core according to its documentation.

## Installing and Starting Mosquitto (MQTT Broker)

Mosquitto is a popular open-source MQTT broker. You can install and start it as follows:

### On Ubuntu/Debian

```bash
# Install Mosquitto and its clients
sudo apt update
sudo apt install mosquitto mosquitto-clients

# Start Mosquitto service
sudo systemctl start mosquitto

# (Optional) Enable Mosquitto to start on boot
sudo systemctl enable mosquitto
```

### Verify Mosquitto is Running

```bash
sudo systemctl status mosquitto
```

You should see that the service is "active (running)".

> **Note:**  
> By default, Mosquitto listens on port 1883. You can test the broker using `mosquitto_pub` and `mosquitto_sub` commands.
## Usage

This CLI tool supports two operation modes:

1. **Command Mode**: Execute single commands directly from the command line
2. **Interactive Mode**: Start an interactive shell for multiple operations

### Command Mode

Execute deployment and configuration commands directly from the command line for automated workflows and scripting.

The CLI tool can be used in two ways:
1. **Direct Python execution**: `python3 edgeapp_cli.py <command>`
2. **Installed CLI command** (after `pip install .`): `edgeapp_cli <command>`

#### Deploy AI Model
```bash
# Direct execution
cd tools/edgeapp_cli
python3 edgeapp_cli.py deploy ai <model_file_path> [version]

# Using installed CLI
edgeapp_cli deploy ai <model_file_path> [version]
```

Examples:
```bash
# Direct execution
python3 edgeapp_cli.py deploy ai ./models/detection.pkg 0311030000000123

# Using installed CLI
edgeapp_cli deploy ai ../bin/network.pkg
edgeapp_cli deploy ai ../bin/network.pkg 1.0.0
```

#### Deploy Edge App Module
```bash
# Direct execution
python3 edgeapp_cli.py deploy edge_app <Edge_App_file_path> [module_name]

# Using installed CLI
edgeapp_cli deploy edge_app <Edge_App_file_path> [module_name]
```

Examples:
```bash
# Direct execution
python3 edgeapp_cli.py deploy edge_app ./build/edge_app.wasm my_edge_app

# Using installed CLI
edgeapp_cli deploy edge_app ../bin/edge_app.wasm
edgeapp_cli deploy edge_app ../bin/edge_app.wasm my_module_name
```

#### Undeploy Edge App Modules
```bash
# Direct execution
python3 edgeapp_cli.py undeploy

# Using installed CLI
edgeapp_cli undeploy
```

#### Send Configuration to Edge App
```bash
# Direct execution
python3 edgeapp_cli.py send conf [config_file] [instance_id] [ps=value]

# Using installed CLI
edgeapp_cli send conf [config_file] [instance_id] [ps=value]
```

Examples:
```bash
# Use default configuration.json and auto-detect instance
edgeapp_cli send conf

# Specify configuration file and process state
edgeapp_cli send conf my_config.json ps=1

# Send to specific instance
edgeapp_cli send conf configuration.json 5ad9c7f6-cac0-46da-8007-8e7c5ff99ef7

# Use --process-state flag (alternative syntax)
edgeapp_cli send conf --process-state 2

# Send to specific instance using --instance flag
edgeapp_cli send conf --instance 5ad9c7f6-cac0-46da-8007-8e7c5ff99ef7
```

#### Command Mode Options
```bash
# Direct execution with options
python3 edgeapp_cli.py --mqtt-host 192.168.1.100 --mqtt-port 1883 --http-port 8080 deploy ai model.pkg

# Using installed CLI with options
edgeapp_cli --mqtt-host 192.168.1.100 --mqtt-port 1884 deploy ai model.pkg
edgeapp_cli --http-port 8080 --http-host 192.168.1.50 deploy edge_app app.wasm
```

> **Note:**  
> In command mode, the HTTP server is automatically started during deployment operations to serve files to the edge device. The server will be automatically configured and managed without requiring manual intervention.

## Installation

For convenient command-line usage, you can install the CLI tool as a package:

```bash
# Install from source
cd tools/edgeapp_cli
pip install .

# Install in development mode (for development)
pip install -e .
```

After installation, you can use the `edgeapp_cli` command from anywhere.

### Interactive Mode

Start an interactive shell for multiple operations and real-time monitoring.

```bash
# Direct execution
cd tools/edgeapp_cli
python3 edgeapp_cli.py

# Using installed CLI
edgeapp_cli
```

By default, it starts with:
- MQTT broker: localhost:1883
- HTTP server: [IP-address-of-your-machine]:8000

> **Note:**  
> This CLI tool will try to set your machine's IP address. If CLI tool cannot set the IP address, use `localhost`, but specifying the actual IP address is recommended to ensure the edge device can access the server.

With options:
```bash
# Direct execution
python3 edgeapp_cli.py --mqtt-host 192.168.1.100 --mqtt-port 1883 --http-port 8080

# Using installed CLI
edgeapp_cli --mqtt-host 192.168.1.100 --mqtt-port 1883 --http-port 8080
```

### Interactive Mode Commands

### Interactive Mode Commands

After startup, the following commands are available in the interactive shell:

#### Deploy AI Model to the Edge Device
```
> deploy ai <model_file_path> [version]
```

Example:
```
> deploy ai ./models/detection.pkg 0311030000000123
```

#### Deploy Edge App Module to the Edge Device
```
> deploy edge_app <Edge_App_file_path> [module_name]
```

Example:
```
> deploy edge_app ./build/edge_app.wasm my_edge_app
```

#### Check Deployed Items
```
> list
```

Shows deployed AI models and Edge App modules with their status:
- `detected from device` - Confirmed deployed on the device via MQTT messages
- `managed by CLI` - Recorded by CLI but not confirmed from device

Example output:
```
=== Deployed Items ===
AI Models
  - detection_model (v0311030000000123) (detected from device)
Edge App Modules
  - ebbda8a9-6f9b-4b64-bfea-c6687607bbd5 (detected from device)
```

Note: Edge App modules are displayed using their instance ID as that's how they are actually managed on the device.

#### Send Configuration to Edge App
```
> send conf [config_file] [instance] [ps=<value>]
```

Examples:
```
> send conf                                       # Use configuration.json, auto-detect instance
> send conf ps=1                                  # Use configuration.json, ps=1, auto-detect instance
> send conf configuration.json                    # Specify config file, auto-detect instance
> send conf configuration.json ps=1               # Specify config file + process_state=1
> send conf configuration.json <instance_id>      # Send to specific instance
```

#### Undeploy Edge App Module
```
> undeploy
```

#### Show Deployed Edge App Instances
```
> instances
```

Shows detailed information about deployed Edge App instances including deployment ID, reconcile status, and instance details.

#### Show Server Status
```
> server status
```

#### Restart HTTP Server
```
> server retry [port]
```

#### Enable/Disable Verbose MQTT Logging
```
> log on
> log off
```

#### Show Recent MQTT Messages
```
> messages
```

#### Show MQTT Status and Attributes
```
> status
```

#### Show Command History
```
> history
```

#### Show Help
```
> help
```

#### Exit CLI
```
> exit
```

## How It Works

1. **HTTP Server**: Places files in upload directory and makes them accessible via HTTP
   - **Interactive Mode**: HTTP server is started during CLI initialization and runs continuously
   - **Command Mode**: HTTP server is automatically started during deployment operations and managed transparently
2. **MQTT Broker**: Uses MQTT to communicate with Edge Device and send deployment information
   - **Interactive Mode**: Waits for device initialization before accepting commands
   - **Command Mode**: Skips device initialization wait for faster command execution
3. **File Management**: Calculates file hash values to ensure uniqueness and proper deployment tracking

## Directory Structure

```
tools/edgeapp_cli/
├── edgeapp_cli.py        # Main CLI tool
├── webserver.py          # HTTP server
├── mqtt.py               # MQTT related functions
├── src/
│   ├── ai_models.py      # AI model definitions
│   ├── modules.py        # Edge App module definitions
│   └── interface.py      # MQTT communication interface
└── server_dir/           # HTTP server file storage
```

## Notes

- MQTT client must be running on Raspberry Pi side
- Files are automatically copied to `server_dir/` directory
- Connection to MQTT broker is required (may need separate startup)


