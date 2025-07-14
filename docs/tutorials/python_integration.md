# Python Modules Integration

## Overview

The Aitrios Edge SDK provides
the boilingplate to easily develop
a python module to run on Wedge Agent.

> [!Warning]
> This is currently under development,
> and further development will take place in
> Wedge SDK and Aitrios SDK.

## Getting Started

### Development Environment

Prior to any python module deployment,
ensuret that your environment is setup.
The tutorial was developed with the following setup:

* a Raspberry Pi 5 with a IMx500 AiPicam
  connected on local network
* a PC with local-console
  connected on local network

#### Raspberry Pi

The Raspberry Pi must have
the Aitrios SDK,
Wege Agent and
Wedge SDK installed:

* Clone Aitrios edgeapp SDK [repo](https://github.com/SonySemiconductorSolutions/aitrios-sdk-edge-app)
  `git clone git@github.com:SonySemiconductorSolutions/aitrios-sdk-edge-app.git`
  The package needs to be installed in the system:
  `sudo pip install aitrios-sdk-edge-app/libs/python --break-system-package`
* Download and install Wedge Python SDK from
  [debian package](https://github.com/midokura/wedge-agent-oss-playground/releases/download/releases%2Fv1.37.0/python3-wedge_0.2-1_arm64.deb):
* Download and install Wedge Agent from
  [debian package](https://github.com/midokura/wedge-agent-oss-playground/releases/download/releases%2Fv1.37.0/wedge-agent-1.36.0_arm64.deb):
* Configure wedge-agent for the local network by editing
  `/lib/systemd/system/wedge-agent.service`
  Edit the env var EVP_MQTT_HOST with your local PC IP:
  `Environment=EVP_MQTT_HOST=<Your Local PC IP>`

#### Local PC

* Install `local-console` from this
  [branch](https://github.com/midokura/local-console/tree/experimental/release/v3.0.x).
  
  Clone project and install:

```
git clone -b experimental/release/v3.0.x git@github.com:midokura/local-console.git
cd local-console
pip install -e local-console
```

* Clone repo with sample demo:

```
git clone git@github.com:SonySemiconductorSolutions/aitrios-sdk-edge-app-dev.git
```

### Development

This tutorial explains how to transform
a `Picam2` example into a fully working
application for *Aitrios* connecting to `Local-Console`.

Let's take the following
[imx500_object_detection_demo.py](https://github.com/raspberrypi/picamera2/blob/main/examples/imx500/imx500_object_detection_demo.py)
sample code to work with,
and integrate Aitrios step by step.

> [!Note]
> Current official script contains some issues
> that must be fixed prior to development.
> See [#1164](https://github.com/raspberrypi/picamera2/issues/1164)

1. Patch current example with fix:

```patch
 --- a/examples/imx500/imx500_object_detection_demo.py
 +++ b/examples/imx500/imx500_object_detection_demo.py
 @@ -25,7 +25,7 @@ def parse_detections(metadata: dict):
     """Parse the output tensor into a number of detected objects, scaled to the ISP output."""
     global last_detections
     bbox_normalization = intrinsics.bbox_normalization
 -    bbox_order = intrinsics.bbox_order
 +    bbox_order = args.bbox_order
     threshold = args.threshold
     iou = args.iou
     max_detections = args.max_detections
 ```

2. Import aitrios sample helper app

Add, at the beginning of the file:

```python
from edgeapplib.utils import encode_picam_image
from edgeapplib.app import EdgeApp
```

3. Create the app

Before executing the runtime loop,
create the app:

```python
    app = EdgeApp("detection")
```

4. Update runtime

The runtime needs to exit
if the app is exiting
and to stop the Edge Device before exiting.

The loop can be exited
when the `app.is_running` becomes `False`:

```python
    last_results = None
    picam2.pre_callback = draw_detections
    while app.is_running:
        last_results = parse_detections(picam2.capture_metadata())

    picam2.stop()
```

5. Update `draw_detections` function
   to use the app
   to stream the image
   and inference results.

> [!Note]
> The image needs to be converted
> to JPG and encoded to bytearray.
> As a convenience, the util function `encode_picam_image`
> can be used to that matter.

```python
def draw_detections(request, stream="main"):
    data = encode_picam_image(request)
    app.process_stream(data, last_results)
```

### Usage

> [!Note]
> While Local Console GUI
> is not sending the direct commands
> to the edge app,
> a script needs to be used
> to forward commands to the expected module instance.

On Raspberry Pi:

1. Make sure the agent service is started:
   `sudo systemctl status wedge-agent`

On local PC:

1. Start the forwarder script.
   In Aitrios edgeapp dir, run:
   `python tools/mqtt_forwarder.py`
2. Start local console:
   `local-console gui`

From local-console:

1. In Application tab, select the
   `sample_app/detection/python/imx500_object_detection_wedge_demo.py` file
   and click `Deploy`
2. In Configuration tab:
    * Type: `Detection`
    * Configuration: from local console repo:
      `sample-apps/detection/configuration.json`
    * Click `Apply Configuration`
3. In Streaming tab: click `Streaming`

The agent should now start streaming data to the local-console.
