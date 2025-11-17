# Edge Application SDK for AITRIOS™

[![CI](https://github.com/SonySemiconductorSolutions/aitrios-sdk-edge-app/workflows/CI/badge.svg)](https://github.com/SonySemiconductorSolutions/aitrios-sdk-edge-app/actions/workflows/ci.yaml)
[![License](https://img.shields.io/badge/license-Apache%202.0-blue.svg)](LICENSE)
[![GitHub release](https://img.shields.io/github/release/SonySemiconductorSolutions/aitrios-sdk-edge-app.svg)](https://github.com/SonySemiconductorSolutions/aitrios-sdk-edge-app/releases)
[![GitHub issues](https://img.shields.io/github/issues/SonySemiconductorSolutions/aitrios-sdk-edge-app.svg)](https://github.com/SonySemiconductorSolutions/aitrios-sdk-edge-app/issues)
[![GitHub stars](https://img.shields.io/github/stars/SonySemiconductorSolutions/aitrios-sdk-edge-app.svg)](https://github.com/SonySemiconductorSolutions/aitrios-sdk-edge-app/stargazers)

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

### Sample Application for C/C++


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
| [lp_recog] | Detects license plates on IMX500 and recognizes license plate characters on CPU(TFLite). |
| [ssl]      | Demonstrates SSL/TLS communication for secure data exchange with both SSL client and server functionality. |

[passthrough]: sample_apps/passthrough
[classification]: sample_apps/classification
[detection]: sample_apps/detection
[posenet]: sample_apps/posenet
[segmentation]: sample_apps/segmentation
[switch_dnn]: sample_apps/switch_dnn
[draw]: sample_apps/draw
[perfbench]: sample_apps/perfbench
[lp_recog]: sample_apps/lp_recog
[ssl]: sample_apps/ssl

---

## Advanced Optimization

### WebAssembly AOT Compilation

You can optimize Edge Application WebAssembly files for various processor architectures including Xtensa, ARM, x86, RISC-V, and more. This optimization uses AOT (Ahead-of-Time) compilation to improve runtime performance, reduce memory footprint, and enable platform-specific features.

**Key Features:**
- Build [WAMR Compiler](https://github.com/bytecodealliance/wasm-micro-runtime/tree/main/wamr-compiler) for your target architecture
- Optimize WebAssembly files with wamrc compiler for specific targets
- Support for multiple architectures: Xtensa, ARM, AArch64, x86_64, RISC-V, etc.
- Enable XIP (Execute In Place) mode for reduced memory usage on embedded systems

**Example Command:**
```bash
# Enable XIP feature for Xtensa target
wamrc --target=xtensa --xip --enable-multi-thread --size-level=0 -o bin/edge_app.aot bin/edge_app.wasm
```

---

#### Run and Debug apps

You can run and debug the built Edge Application **without deploying it to the Edge device** by following way:

Target devices: **Ubuntu (amd64), Raspberry Pi (aarch64)**

Before running this Edge Application, please ensure that **Netcat** is installed on your Target Device.  
Netcat is used to send configuration data to the application.  

```sh
sudo apt update
sudo apt install -y netcat-traditional
```

#### Steps 

##### 1. Install senscord-libcamera

Install the package using the Debian package repository according to your system:

- **For Ubuntu 22.04 (Jammy/amd64)**  
  ```sh
  wget http://midokura.github.io/debian/evp-archive-keyring_jammy_amd64.deb
  sudo dpkg -i ./evp-archive-keyring_jammy_amd64.deb 
  sudo apt update
  sudo apt install senscord-libcamera
  ```

- **For Ubuntu 20.04 (Focal/amd64)**  
  ```sh
  wget http://midokura.github.io/debian/evp-archive-keyring_focal_amd64.deb
  sudo dpkg -i ./evp-archive-keyring_focal_amd64.deb 
  sudo apt update
  sudo apt install senscord-libcamera
  ```

- **For Raspberry Pi (Bookworm/aarch64)**  
  ```sh
  wget http://midokura.github.io/debian/evp-archive-keyring_bookworm_arm64.deb
  sudo dpkg -i ./evp-archive-keyring_bookworm_arm64.deb 
  sudo apt update
  sudo apt install senscord-libcamera
  ```


##### 2. Run Edge Application

If the **Target Device is Ubuntu**, run the following command:

  ```sh
  /opt/senscord/run_iwasm.sh -d pc /path/to/your_edge_app.wasm
  ```

If the **Target Device is Raspberry Pi**, run the following command:

  ```sh
  /opt/senscord/run_iwasm.sh /path/to/your_edge_app.wasm
  ```


Then Edge Application waits the command to start inference

##### 3. Start Inference

In another terminal:

```sh
cat configuration.json | nc localhost 8080
```
You can use the sample configuration file in the `sample_apps/$NAME_OF_APP/configuration` directory.


##### 4. Check the Results

When you run the application at step 2, **two folders** will be automatically created under your current directory:

- **image**: Stores the full images or cropped images generated during the inference process.
- **Inference**: Contains the output data (e.g., Output Tensors) from the AI inference.

---

## Building the Documentation

You can view the documentation locally:

### Setup

Install uv.
```bash
curl -LsSf https://astral.sh/uv/install.sh | sh
```

Create virtual environment.
```bash
uv venv
```
Activate virtual environment.
```bash
source .venv/bin/activate
```
Install required packages.
```bash
uv pip install mkdocs mkdocs-material
uv pip install mkdocs-static-i18n # for supporting multiple language
```

### View Documentation Locally

Start local server.
```bash
uv run mkdocs serve
```

Then open http://localhost:8000 in your browser.


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
