# Edge App Python Bindings Prototype

This project provides Python bindings for the Edge App SDK.
The Python package can be built using the real SensCord libraries and the mocked EVP libraries.
This enables usage from Python without requiring the EVP Agent.

## Install SensCord
Follow these instructions to download the Debian package:
[Debian Package](https://github.com/SonySemiconductorSolutions/aitrios-sdk-edge-app/blob/main/docs/tutorials/raspi_localconsole.md#debian-packages)

Then install SensCord using `apt`:
```sh
sudo apt update
sudo apt install senscord-libcamera python3-senscord

```

## Build

First, set up a Python virtual environment:

```sh
python -m venv venv
. venv/bin/activate
```

Then, build and install the Edge App SDK package with SensCord and the mocked EVP Agent:

Run the following command from the same directory as this README.

```sh
EVP_MOCK=1 SENSCORD_INSTALL_PATH=/opt/senscord \
/opt/senscord/share/senscord/setup_env.sh \
pip install .
```

### Environment Variables

- `SENSCORD_INSTALL_PATH`: Path to the SensCord. (typically `/opt/senscord`).
- `EVP_MOCK`: Set to `1` enable the mocked EVP Agent, or `0` to use the real agent.
- `setup_env.sh`: This script sets up the environment needed for building and running the SDK.

⚠️ `setup_env.sh` is required because the build process generates a Python stub file (`.pyi`) for the bindings. This requires the newly built Python module to be loaded. Without the correct environment (set by `setup_env.sh`), it may fail to find the SensCord libraries.

## Install `nc` (Netcat) on Raspberry Pi

Install `netcat-traditional`, which provides the `nc` command:

```sh
sudo apt update
sudo apt install netcat-traditional
```

## How to Run the Example

Once installed, you can use the `edge_app_sdk` module in Python:
```python
import edge_app_sdk
```

An example Edge App is provided in `example/simple_edge_app.py`. Run it with:
```sh
/opt/senscord/share/senscord/setup_env.sh python example/simple_edge_app.py
```

The app will start to run in the IDLE state (1).

To transition to another state, send a new configuration:

```sh
cat example/configs/configuration.json | nc localhost 8080
```

This sends a DTDL-based configuration to the Edge App.

To destroy the app, use the following command:

```sh
echo "" | nc localhost 8080
```

> ℹ️ Make sure port 8080 is not being used by another process.

You can check whether the port is in use with:

```sh
sudo lsof -i :8080
```

📝 Note: `simple_edge_app.py` saves images and metadata to a local folder, but the image data is not encoded to JPEG or BMP format.

## How to Run the OpenCV Example

Another example demonstrates how to use OpenCV to display the images with inference results.

The example is located in `example/privacy_mask_edge_app.py`. Run it with:
```sh
/opt/senscord/share/senscord/setup_env.sh python example/privacy_mask_edge_app.py
```

Then, send a new configuration as described in the previous section.
