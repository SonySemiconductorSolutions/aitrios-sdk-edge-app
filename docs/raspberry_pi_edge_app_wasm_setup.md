## Overview

The guide provides the instructions on how to run the Edge App on a Raspberry Pi.

<img src="img/raspi_edge_app_wasm_setup.png">

## Get  Related Raspberry Pi Software

Prerequisites
Update and upgrade your system:

```
sudo apt update && sudo apt full-upgrade
```
Install the IMX500 firmware
```
sudo apt install imx500-all
```
Reboot the System
```
sudo reboot now
```


## Install senscord-libcamera

```
wget http://midokura.github.io/debian/evp-archive-keyring_bookworm_arm64.deb
sudo dpkg -i ./evp-archive-keyring_bookworm_arm64.deb
sudo apt update
sudo apt install senscord-libcamera
```
Once extracted, related files are located under `/opt/senscord`

## Download the official EdgeApp Wasm from the release page

[EdgeApp public repo](https://github.com/SonySemiconductorSolutions/aitrios-sdk-edge-app/releases/download/1.1.1/sample_edge_app_detection_wasm_v2_1.1.1.zip)

## Start MQTT broker and HTTP server

Start MQTT broker in your PC
```bash
mosquitto -c mosquitto.conf -v
```
`mosquitto.conf`:
```
allow_anonymous true
listener 1883
```
and start HTTP server where the wasm is located
```bash
python3 -m http.server
```


## Start EVP agent
```
vim /opt/senscord/run_agent.sh
```
Change the IP address to where the MQTT broker running
```sh
EVP_DATA_PATH="bin/evp_data"
rm -rf "$EVP_DATA_PATH"
if [ ! -d "$EVP_DATA_PATH" ]; then
  mkdir -p "$EVP_DATA_PATH"
  echo "Directory created: $EVP_DATA_PATH"
else
  echo "Directory already exists: $EVP_DATA_PATH"
fi
export EVP_MQTT_HOST=192.168.1.106       <----- modify to your host IP
export EVP_MQTT_PORT=1883
export EVP_DATA_DIR=$EVP_DATA_PATH
# Uncomment the following lines to connect AITRIOS Console
# export EVP_MQTT_HOST=mqtt.evp2-tb.demo-jp.midokura.com
# export EVP_MQTT_PORT=8883
# export EVP_HTTPS_CA_CERT=/home/pi/evp_data/IncludedRootsPEM_20240723.txt
# export EVP_MQTT_TLS_CA_CERT=/home/pi/evp_data/root-ca.crt
# export EVP_MQTT_TLS_CLIENT_CERT=/home/pi/evp_data/client-cert.pem
# export EVP_MQTT_TLS_CLIENT_KEY=/home/pi/evp_data/client-key.pem
export EVP_IOT_PLATFORM=tb
./share/senscord/setup_env_cam.sh ./bin/evp_agent -l ./lib/libsenscord_wamr.so -l ./lib/libesf-device-wamr.so
```
then execute
```
/opt/senscord/run_agent.sh
```

## Deploy Wasm Application

   ```bash
    mosquitto_pub \
      -h localhost -p 1883 \
      -t v1/devices/me/attributes \
      -m '{
        "deployment": {
          "deploymentId": "adec05b9275e1561ef4bf0ae0e9562386533adda7ae12fcfc1467332fe0f435f",
          "instanceSpecs":{
            "node": {
              "moduleId": "node-721f9",
              "subscribe": {},
              "publish": {}
            }
          },
          "modules": {
            "node-721f9": {
              "entryPoint": "main",
              "moduleImpl":"wasm",
              "downloadUrl": "http://192.168.11.24:8000/edge_app_detection.wasm",
              "hash":"76c2a97ebccf44ef1be19d0bd356f0f67f3021c815267bcf8599dd73a39fb5b8"
            }
          },
          "publishTopics": {},
          "subscribeTopics": {}
        }
      }'
   ```
   Example of a script:
   ```bash
    filename="edge_app_detection.wasm"

    hash=$(openssl sha256 -r "$filename" | cut -d' ' -f1)

    json=$(jq -n \
        --arg hash "$hash" \
        --arg filename "$filename" \
        '{
            "deployment": {
                "deploymentId": "33169145-8EB1-45AE-8267-35427323515E",
                "instanceSpecs": {
                    "node": {
                        "moduleId": "node-721f9",
                        "publish": {},
                        "subscribe": {}
                    }
                },
                "modules": {
                    "node-721f9": {
                        "entryPoint": "main",
                        "moduleImpl": "wasm",
                        "downloadUrl": "http://192.168.11.24:8001/\($filename)",
                        "hash": $hash
                    }
                }
            }
        }')

     mosquitto_pub -h localhost -p 1883 -t v1/devices/me/attributes -m "$json"
   ```

## Verify Deployment

Check `evp-agent` logs: you should see `status: ok` for your deployment ID.


## Change Edge App Status to Running

```bash
mosquitto_pub -h localhost -p 1883 \
  -t v1/devices/me/attributes \
  -m '{"configuration/node/edge_app": "{\"req_info\":{\"req_id\":\"run1\"},\"common_settings\":{\"process_state\":2}}"}'
```
Example of a script with configurations:
```bash
internal_json=$(jq -n \
'{
    "req_info": {
        "req_id": "process_state2"
    },
    "common_settings": {
        "log_level": 4,
        "port_settings": {
            "metadata": {
                "path": "metadata",
                "method": 2,
                "enabled": true,
                "endpoint": "http://192.168.11.24:8080",
                "storage_name": ""
            },
            "input_tensor": {
                "path": "image",
                "method": 2,
                "enabled": true,
                "endpoint": "http://192.168.11.24:8080",
                "storage_name": ""
            }
        },
        "process_state": 2,
        "codec_settings": {
            "format": 1
        },
        "inference_settings": {
            "number_of_iterations": 0
        },
        "number_of_inference_per_message": 1
    },
    "custom_settings": {
         "ai_models": {
            "detection": {
                "ai_model_bundle_id": "012345",
                "parameters": {
                    "max_detections": 2,
                    "threshold": 0.5,
                    "input_width": 300,
                    "input_height": 300,
                }
            }
        },
        "metadata_settings": {
            "format": 1
        }
    }
}' | jq -c .)

json=$(jq -n --arg internal_json "$internal_json" \
'{
    "configuration/node/edge_app": $internal_json
}' | jq -c .)

mosquitto_pub -h localhost -p 1883 -t v1/devices/me/attributes -m "$json"
```

## Undeploy Edge App

```bash
mosquitto_pub -h localhost -p 1883 \
  -t v1/devices/me/attributes \
  -m '{"deployment": {"deploymentId":"test","instanceSpecs":{},"modules":{},"publishTopics":{},"subscribeTopics":{}}}'
```

## Download AI models
EdgeApp SDK supports the deployment of multiple AI models. In addition to AI models deployed on the imx500, it can also support AI models running on CPU/GPU via wasi-nn. These AI models need to be downloaded in advance. The SDK provides DTDL commands for downloading the models.
### How to download the AI Model
Author the DTDL command. The format of this command is defined below in [edge_app_lp_recog_interface](../sample_apps/lp_recog/package/edge_app_lp_recog_interface.json). And one example for how to write the [configuration](../sample_apps/lp_recog/configuration/configuration.json) is
```bash
  "common_settings": {
      "number_of_inference_per_message": 1,
        "ai_models": [
        {
          "name": "lp_recognition",
          "target": "cpu",
          "url_path": "http://localhost:8080/models/LPR.tflite",
          "hash": "6fb13ba628bd4dbf168c33f7099727f6cb21420be4fd32cdc8b319f2d0d736cf"
        }
      ]
  },
```
### API list for downloading
| Function     | Description                                                                 |
| -------------| --------------------------------------------------------------------------- |
| `name`       | The name field serves as the index for the model on the edge side. The EdgeAppCore API will use it to load the model, but users do not need to concern themselves with the physical storage location of the model. |
| `target`     | The target field specifies the execution method (or runtime environment) for the model, the target value can be: `cpu`, `gpu`, `npu`|
| `url_path`   | Before preparing to send this command, please place your AI model in the LAN or WAN, ensuring it is accessible from the edge device. The url_path field specifies this network address.|
| `hash`       | The hash field stores the SHA-256 checksum of the model file, which is used for verification during deployment. |

**NOTICE:** The file extension in the url_path is significant, as it determines which AI backend will be used to interpret and execute the AI model. The currently supported backends are ONNX and TFLite. The corresponding file extensions are as follows:
| File extension name       | to be loaded by the engine of |
| --------------| ---------------------------------------------- |
| `.onnx`       | The AI model in ONNX format will be loaded and executed by the ONNX Runtime inference engine.                             |
| `.tflite`     | The tflite file, which is a TensorFlow Lite format model, will be loaded and executed by the TFLite interpreter/engine.   |
| `.gguf`       | Load and run the LLaMA-compatible Large Language Model file using the LLaMA engine.                                       |
| `.bin`+`.xml` | The OpenVINO model will be loaded and executed using the OpenVINO Runtime. `.bin`+`.xml` indicates the model consists of two files with the same name but different extensions. The AI engine only needs to know the name to load two files correctly. |


So in the example, the LPR.tflite will be loaded by tflite engine.

**WARNING:** If the url_path does not specify a file extension, the system will default to using the ONNX engine. If the model is not in the ONNX format, the inference will fail.

### How to start inference using downloaded AI model
The downloaded AI model can be loaded using the EdgeAppCore API. The EdgeAppCore API knows the storage location of the recently downloaded model. We only need to provide the name of this AI model, which is specified by the name field mentioned above.
```bash
  EdgeAppCoreModelInfo model_info[2] = {
    {"000000", EdgeAppCoreTarget::edge_imx500, {}, {}},
    {"lp_recognition", EdgeAppCoreTarget::edge_cpu, &mean_values, &norm_values}
  };
  ...
  if (LoadModel(model_info[1], ctx_cpu, &shared_ctx) != EdgeAppCoreResultSuccess) {
    LOG_ERR("Failed to load model.");
    return -1;
  }
```
For details on wasi-nn programming, please refer to [EdgeAppCore_API_specification](api/EdgeAppCore_API_specification.md), chapter Usage Example