# Edge Application SDK for AITRIOS™

This repository provides SDKs and libraries to simplify the development of Edge Applications across various devices. Using this SDK, you can efficiently develop AI applications on Edge devices, such as handling AI inference results from the IMX500 and processing image data.

## Features
- Easy development support with Python.
- Convert C/C++ code to WASM, enabling it to run on various Edge devices.
- Send AI inference results to a Local Console.
- Supports event-driven development, allowing developers to define handlers for each event.

## Getting Started

### Cloning the Repository
This project requires external libraries such as **FlatBuffers**, **Parson**, **Base64** and **Pybind11** you need to ensure that all submodules are initialized and updated. Use the following command: 
`git submodule update --init --recursive`

### Install Local Console
This Edge Application SDK is designed to connect to a Local Console. You need to set up the Local Console environment in advance by following the instructions here:
To avoid compatibility issues, please ensure you use version v4.0.18-sss.
[Local Console](https://github.com/SonySemiconductorSolutions/local-console/tree/v4.0.18-sss)

### Sample Application for Python on Raspberry Pi + AI Camera

The sample Edge Application Python runs on a Raspberry Pi with an AI Camera. First, follow the instructions on the Picamera2 GitHub repository to install the necessary tools:
[Picamera2](https://github.com/raspberrypi/picamera2)

#### Install Edge App Python Library

```python
python3 -m venv .venv --system-site-package
source .venv/bin/activate
pip install -e ./libs/python
```

#### Run Sample Edge Application Python
```python
python3 sample_apps/classification/python/imx500_classification_local_console_demo.py --model /usr/share/imx500-models/imx500_network_efficientnet_bo.rpk --softmax --mqtt_host=localhost --mqtt_port=1883
```

### Sample Application for C/C++

Currently, C/C++ applications do not run on the Raspberry Pi with the AI Camera.

#### Build apps

You can build edge applications using the command below.

```sh
make CMAKE_FLAGS="-DAPPS_SELECTION=${NAME_OF_APP}"
```

| NAME_OF_APP      | Description   |
|------------------|------------|
| [passthrough]    | Outputs Input tensors and Output tensors obtained from Edge Devices without any processing.|
| [classification] | Outputs encoded inference results into FlatBuffers after post-processing for classification task on IMX500.|
| [detection]      | Outputs encoded inference results into FlatBuffers after post-processing for objectdetection task on IMX500.|
| [posenet] | Outputs encoded inference results into FlatBuffers after post-processing for posenet task on IMX500.|
| [segmentation]   | Outputs encoded inference results into FlatBuffers after post-processing for segmentation task on IMX500.|
| [switch_dnn]     | Sample application for switching between multiple DNNs on IMX500.|
| [draw]           | Sample application for drawing bounding box on input tensor from IMX500.|
| [perfbench]      | Sample Application for measuring performance with Debug Code.|

[passthrough]: sample_apps/passthrough
[classification]: sample_apps/classification
[detection]: sample_apps/detection
[posenet]: sample_apps/posenet
[segmentation]: sample_apps/segmentation
[switch_dnn]: sample_apps/switch_dnn
[draw]: sample_apps/draw
[perfbench]: sample_apps/perfbench


#### Execute Wasm Application in Place

Before running this Edge Application, please ensure that **Netcat** is installed on your Target Device.
Netcat is used to send configuration data to the application.  
It’s also a good idea to install OpenCV for visualization.

To install Netcat, run:

   ```sh
   sudo apt update
   sudo apt install -y netcat-traditional libopencv-dev python3-opencv
   ```

##### Steps

1. **Download and install senscord_libcamera.deb**  
   Download the `senscord_libcamera.deb` package from the release page according to your device architecture and install it using:


   ```sh
   wget https://github.com/SonySemiconductorSolutions/aitrios-sdk-edge-app/releases/download/1.1.6/senscord-libcamera_1.0.5_arm64.deb

   sudo apt install ./senscord-libcamera_*.deb
   ```


   | Arch      | filename   |
   |------------------|------------|
   | arm64    | senscord-libcamera_1.0.5_arm64.deb |
   | ubuntu 22.04    | senscord-libcamera_1.0.5_u22_amd64.deb |
   | ubuntu 20.04    | senscord-libcamera_1.0.5_u20_arm64.deb |



2. **Run the application**  
    Execute the following command from another terminal.
You can find sample configuration files in the sample_apps/"APPNAME"/configuration directory.
   
   To use the real frame from camera, use the script as default setting.
   ```sh
   cd /opt/senscord
   ./run_iwasm.sh </path/to/your_edge_app.wasm>
   ```
   To use the captured frame, specify pc as a device
   ```sh
   cd /opt/senscord
   ./run_iwasm.sh -d pc </path/to/your_edge_app.wasm>
   ```

3. **Start Inference**  
    Execute the command from the other terminal. You can find the sample configuration from sample_apps/"APPNAME"/configuration. 
   ```sh
   cat configuration.json | nc localhost 8080
   ```

4. **Check the result**  
   When you run this application, **two folders** will be automatically created:

- **image**: Stores the full images or cropped images generated during the inference process.
- **Inference**: Contains the output data (e.g., Output Tensors) from the AI inference.



## Get support
- [Contact us](https://support.aitrios.sony-semicon.com/hc/en-us/requests/new)

## See also
- ["**Console for AITRIOS**"](https://console.aitrios.sony-semicon.com/)
- ["**Console Manual**"](https://developer.aitrios.sony-semicon.com/en/edge-ai-sensing/documents/console-v2/console-user-manual/)
- ["**Cloud SDK Visualization Tutorial**"](https://github.com/SonySemiconductorSolutions/aitrios-sdk-visualization-ts/tree/main/docs/development-docs/)
- ["**Developer Site**"](https://developer.aitrios.sony-semicon.com/en/)
- ["**AI Camera**"](https://www.raspberrypi.com/documentation/accessories/ai-camera.html)

## Trademark
- [Read This First](https://developer.aitrios.sony-semicon.com/en/edge-ai-sensing/documents/console-v2/read-this-first/)

## Security
Before using Codespaces, please read the Site Policy of GitHub and understand the usage conditions. 

## AI Ethics
This SDK is optimized for use with Sony's AITRIOS™ (https://developer.aitrios.sony-semicon.com/en/). If you are willing to take part in the AITRIOS and use services provided through AITRIOS, you must sign up with https://developer.aitrios.sony-semicon.com/en/ and comply with AITRIOS Terms of Use and other applicable terms of use in addition to the license terms of this SDK.

AITRIOS is a one-stop platform that provides tools and environments to facilitate software and application development and system construction.

Sony, with the aim of utilizing AI technology to enrich people's lifestyles and contribute to the development of society, will pursue accountability and transparency while actively engaging in dialogue with stakeholders. Sony will continue to promote responsible AI in order to maintain the trust of products and services by stakeholders. 

Users of this SDK should refer to and understand our thoughts and initiatives about AI. You can learn more [here](https://www.sony.com/en/SonyInfo/sony_ai/responsible_ai.html), including Sony Group AI Ethics Guidelines. 
