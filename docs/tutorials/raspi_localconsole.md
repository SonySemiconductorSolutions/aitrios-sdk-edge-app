# How to Set Up Raspi AI Camera + Local Console

## Hardware needed

- Edge Device:
  - Raspberry AI Camera
  - Raspberry Pi compatible with AI Camera
- Server/Host:
  - A computer (can be your work computer)

## Local Console

Download and install Local Console on your machine. Follow the instructions [here](https://github.com/SonySemiconductorSolutions/local-console).

## Edge Device

 Make sure the AI camera is properly installed and working according to the [official AI Camera documentation](https://www.raspberrypi.com/documentation/accessories/ai-camera.html).

### Dependencies

Install all the dependencies necessary for Raspi AI Camera:

```sh
sudo apt install imx500-all python3-msgpack
```

### Debian packages

Download `evp-archive-keyring.deb` from [here](http://midokura.github.io/debian/evp-archive-keyring.deb) and install it in your Raspberry Pi with:

```
wget http://midokura.github.io/debian/evp-archive-keyring.deb
sudo dpkg -i evp-archive-keyring.deb
sudo apt update
```

Install all required components using `apt`:

```
sudo apt install evp-agent python3-evp-app senscord-libcamera python3-senscord system-stub
```

## Configure Edge Device

On the Edge Device, edit the file `/opt/senscord/run_agent.sh` and modify the following lines to point to the correct MQTT server and port in the local-console server:

```
export EVP_MQTT_HOST=192.168.1.106
export EVP_MQTT_PORT=1883
```

Also, on the Edge Device, edit the file `/etc/system-app/env.conf` and edit the following lines to point to the same correct MQTT server and port in the local-console server:

```
EVP_MQTT_HOST=192.168.1.106
EVP_MQTT_PORT=1883
```

Set `EVP_MQTT_HOST` to the IP address or the hostname of the local-console.

Set `EVP_MQTT_PORT` to the port assigned to the device in local-console.

The values on both files (`/opt/senscord/run_agent.sh` and `/etc/system-app/env.conf`) should match.

## Start the services

Enable and start the services now:

```sh
sudo systemctl enable --now senscord
sudo systemctl enable --now evp-agent-senscord
sudo systemctl enable --now system-stub
```

## Usage with Local Console

### 1. Prepare the AI model

First, create a virtual environment to run the Python script on Raspberry pi:
```bash
python3 -m venv .venv
. ./.venv/bin/activate
```
Next, install `ultralytics`(Source: https://docs.ultralytics.com/integrations/sony-imx500/):

```bash
sudo apt install imx500-tools
pip install ultralytics
```

Then, run the following Python script:

```python:export_yolov8n.py
from ultralytics import YOLO

# Download pre-trained YOLOv8n model
model = YOLO("yolov8n.pt")

# Export to IMX500 format
model.export(format="imx", data="coco8.yaml", imgsz=(544, 544))
```

We need to generate rpk format AI model for IMX500.
```bash
imx500-package -i yolov8n_imx_model/packerOut.zip -o yolov8n_imx_model
```
  This .rpk is now ready to be deployed this using Local Console


### 2. Deploy the Edge application
    
You can use either formats, the Python or the WASM edge applications (they are the same)
  - WASM: https://github.com/SonySemiconductorSolutions/aitrios-sdk-edge-app/releases/download/1.1.6/edge_app_detection_arm64.aot_1.1.6.zip
    - Once extracted zip file, deploy edge_app_detection.arm64.aot using the Local Console.
  - Python: https://github.com/SonySemiconductorSolutions/aitrios-sdk-edge-app/blob/main/libs/py/example/object_detection_edge_app_yolov8.py

### 3. Apply the configuration file to the Edge Application

  This configuration file can be now deployed using Local Console:
- Configuration File: https://github.com/SonySemiconductorSolutions/local-console/blob/main/sample-apps/detection/configuration.json
