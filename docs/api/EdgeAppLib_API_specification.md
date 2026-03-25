# EdgeAppLib API Walk-through

## Overview
The EdgeAppLib API provides tools for capturing, managing, and exporting sensor data for analysis or integration. It is particularly useful in applications needing efficient real-time data capture and transmission. The API is organized into two main sections:

- **[API for Sensor Data Acquisition](#api-for-sensor-data-acquisition)**: This section offers functions to initialize, start, manage, and configure sensor streams, allowing robust control over data capture.

- **[API for Exporting Data](#api-for-exporting-the-data)**: This section supports asynchronous data export, enabling seamless data transmission to external systems. It includes methods for sending data, managing responses, and configuring network settings.

These APIs streamline sensor data management, from acquisition to export, making them suitable for applications involving IoT, edge processing, and data analytics.

---

## API for Sensor Data Acquisition

The EdgeAppLib Sensor API provides functions for controlling sensor streams and handling sensor data. This includes managing sensor channels, accessing raw data, and configuring stream and channel properties.

### Key Functionalities
- **Stream Management**:
  - Open and close streams with `SensorCoreOpenStream` and `SensorCoreCloseStream`.
  - Start and stop streams with `SensorStart` and `SensorStop`.
  - Retrieve and release frames using `SensorGetFrame` and `SensorReleaseFrame`.

- **Channel and Data Access**:
  - Access channels with `SensorFrameGetChannelFromChannelId`.
  - Retrieve raw data using `SensorChannelGetRawData`.

- **Stream and Channel Properties**:
  - Get or set stream properties via `SensorStreamGetProperty` and `SensorStreamSetProperty`.
  - Configure channel properties using `SensorChannelGetProperty`.
  - Set framerate at which ISP processess sensor data using `SensorStreamSetIspFrameRate`.

These functionalities ensure developers have comprehensive control over sensor data streams.

### API List

| Function                               | Description                             |
| -------------------------------------- | --------------------------------------- |
| `SensorCoreOpenStream`          | Opens the sensor stream                 |
| `SensorCoreCloseStream`         | Closes the sensor stream                |
| `SensorStart`                   | Starts the sensor stream                |
| `SensorStop`                    | Stops the sensor stream                 |
| `SensorGetFrame`                | Gets the current frame from the stream  |
| `SensorReleaseFrame`            | Releases the frame                      |
| `SensorFrameGetChannelFromChannelId` | Gets the channel by ID             |
| `SensorChannelGetRawData`       | Gets raw data from the channel          |
| `SensorStreamGetProperty`       | Gets stream properties                  |
| `SensorStreamSetProperty`       | Sets stream properties                  |
| `SensorChannelGetProperty`      | Gets channel properties                 |
| `SensorGetLastErrorLevel`       | Gets the last error level               |
| `SensorGetLastErrorCause`       | Gets the cause of the last error        |
| `SensorGetLastErrorString`      | Gets error description                  |
| `SensorStreamSetIspFrameRate`   | Sets the ISP FrameRate (typically used on Raspberry Pi)  |

### Usage Example

```cpp
using EdgeAppLib;

EdgeAppLibSensorCore core = 0;
EdgeAppLibSensorStream stream = 0;
int32_t ret = -1;

// Initialize core
if ((ret = SensorCoreInit(&core)) < 0) return ret;

// Open the sensor stream
const char *stream_key = "your key";
if ((ret = SensorCoreOpenStream(core, stream_key, &stream)) < 0) return ret;

// Start streaming
if ((ret = SensorStart(stream)) < 0) return ret;

// Retrieve and process frames
for (int i = 0; i < 10; i++) {
    EdgeAppLibSensorFrame frame;
    if ((ret = SensorGetFrame(stream, &frame, -1)) < 0) return ret;

    EdgeAppLibSensorChannel channel;
    if ((ret = SensorFrameGetChannelFromChannelId(frame, _SENSOR_CHANNEL_ID_INFERENCE_OUTPUT, &channel)) < 0) return ret;

    EdgeAppLibSensorRawData data = {0};
    if ((ret = SensorChannelGetRawData(channel, &data)) < 0) return ret;
    // Access data via data.address, data.size
}

// Streaming Stop
if ((ret = SensorStop(stream)) < 0) return ret;

// Close Sensor
if ((ret = SensorCoreCloseStream(core, stream)) < 0) return ret;

// Exit from Sensor Core
if ((ret = SensorCoreExit(core)) < 0) return ret;

```

---

### API for setting IspFrameRate

#### SensorStreamSetIspFrameRate Description
Sets the ISP (Image Signal Processor) frame rate for the sensor stream. This API allows you to control the frame rate at which the ISP processes images, which must be less than or equal to the camera frame rate.

This API is typically used on platform like Raspberry Pi, where camera frame rate configurations are limited and user wants to adjust the rate at which frames are processed by ISP.

#### Function Signature
```cpp
int32_t SensorStreamSetIspFrameRate(
    EdgeAppLibSensorStream stream,
    const EdgeAppLibSensorIspFrameRateProperty ispFrameRate);
```

#### Parameters

| Parameter | Type | Direction | Description |
|-----------|------|-----------|-------------|
| `stream` | `EdgeAppLibSensorStream` | in | Handle of the sensor stream to configure |
| `ispFrameRate` | `EdgeAppLibSensorIspFrameRateProperty` | in | ISP frame rate property containing numerator and denominator values |

#### EdgeAppLibSensorIspFrameRateProperty Structure

```cpp
struct EdgeAppLibSensorIspFrameRateProperty {
  uint32_t num;    // Numerator of the frame rate
  uint32_t denom;  // Denominator of the frame rate
};
```

The frame rate is calculated as: **Frame Rate (fps) = num / denom**

#### Frame Rate Examples

| Frame Rate | num | denom |
|------------|-----|-------|
| 9.99 fps   | 999 | 100   |
| 29.97 fps  | 2997| 100   |
| 30 fps     | 30  | 1     |


#### Return Values

| Value | Description |
|-------|-------------|
| `0` | Success |
| Negative value | Failure (see error conditions below) |

#### Error Conditions

The function returns an error (`-1`) in the following cases:
- `num` or `denom` is 0
- ISP frame rate exceeds camera frame rate
- Unable to fetch camera frame rate property for validation
- Failed to set the ISP frame rate property

#### Remarks

1. **Frame Rate Constraint**: The ISP frame rate must be less than or equal to the camera frame rate. The function validates this constraint internally by querying the current camera frame rate.

2. **Non-zero Values**: Both `num` and `denom` must be non-zero positive values.

3. **Platform Specific**: This API is particularly useful on Raspberry Pi platforms where ISP frame rate configuration is supported.

4. **Validation**: The function performs automatic validation against the camera's current frame rate setting before applying the ISP frame rate.

5. **Usage Constraint**: This API has to be called before the stream is started. If the stream is already started, then user need to restart the stream by calling Stop/Start Stream for the ISP Framerate to be updated.

#### Usage Example

```cpp
using EdgeAppLib;

EdgeAppLibSensorCore core = 0;
EdgeAppLibSensorStream stream = 0;
int32_t ret = -1;

// Initialize core and open stream
if ((ret = SensorCoreInit(&core)) < 0) return ret;
const char *stream_key = "your_stream_key";
if ((ret = SensorCoreOpenStream(core, stream_key, &stream)) < 0) return ret;

// Configure ISP frame rate to 30 fps
EdgeAppLibSensorIspFrameRateProperty ispFrameRate;
ispFrameRate.num = 30;
ispFrameRate.denom = 1;

ret = SensorStreamSetIspFrameRate(stream, ispFrameRate);
if (ret < 0) {
    // Handle error - ISP frame rate might exceed camera frame rate
    // or other validation failed
    LOG_ERR("Failed to set ISP frame rate: %d", ret);
    return ret;
}

// Start streaming with the configured ISP frame rate
if ((ret = SensorStart(stream)) < 0) return ret;

// ... process frames ...

// Clean up
SensorStop(stream);
SensorCoreCloseStream(core, stream);
SensorCoreExit(core);
```
Refer to sample_app/detection for implementation details.

#### Alternative Usage Example with CPU Model Load
When EdgeAppCore API is used to Load CPU Model, stream is started automatically within LoadModel API. However,if SensorStreamSetIspFrameRate API called during streaming, stream must be restart for the value to take effect. Hence we need to restart the stream after SensorStreamSetIspFrameRate API is called.  

```cpp
using EdgeAppLib;

EdgeAppLibSensorCore s_core = 0;
EdgeAppLibSensorStream s_stream = 0;

EdgeAppCoreCtx ctx_imx500;
EdgeAppCoreCtx ctx_cpu;
EdgeAppCoreCtx *ctx_list[2] = {&ctx_imx500, &ctx_cpu};
EdgeAppCoreCtx *shared_list[2] = {nullptr, &ctx_imx500};

for (int i = 0; i < model_count; ++i) {
    if (EdgeAppCore::LoadModel(models[i], *ctx_list[i], shared_list[i]) !=
        EdgeAppCoreResultSuccess) {
      LOG_ERR("Failed to load model %d.", i);
      return -1;
    } else {
      LOG_INFO("Successfully loaded model %d: %s", i, models[i].model_name);
    }
    LOG_DBG(
        "Model ctx %d: sensor_core=%p, sensor_stream=%p, "
        "graph_ctx=%p, target=%d",
        i, ctx_list[i]->sensor_core, ctx_list[i]->sensor_stream,
        ctx_list[i]->graph_ctx, ctx_list[i]->target);
}
  s_stream = *ctx_imx500.sensor_stream;

// Configure ISP frame rate to 9.99 fps 
EdgeAppLibSensorIspFrameRateProperty ispFrameRate;
ispFrameRate.num = 999;
ispFrameRate.denom = 100;

int result = SensorStreamSetIspFrameRate(s_stream, ispFrameRate);
if (result == 0) {
  LOG_DBG("IspFramerate property set");
} else {
  LOG_ERR("Failed to set IspFrameRate err=%d", result);
}

/*
 Restarting stream to reflect ISP frame rate setting
*/
result = SensorStop(s_stream);
if (result == 0) {
  result = SensorStart(s_stream);
  if (result != 0) {
    LOG_ERR("Failed to start stream for restart %d", result);
  }
} else {
   LOG_ERR("Failed to stop stream for restart %d", result);
}

ispFrameRate = {.num = 0, .denom = 0};
result = SensorStreamGetProperty(s_stream,
                                   AITRIOS_SENSOR_ISP_FRAME_RATE_PROPERTY_KEY,
                                   &ispFrameRate, sizeof(ispFrameRate));
if (result == 0) {
  LOG_INFO("Get IspFramerate property num=%d denom=%d", ispFrameRate.num,
           ispFrameRate.denom);
} else {
  LOG_ERR("Failed to get IspFrameRate err=%d", result);
}

```
Refer to sample_app/lp_recog for implementation details.

---

## API for Exporting the data

The  Data Export provides an interface for managing network communications and asynchronous operations. It facilitates tasks such as sending data, handling asynchronous responses, and managing the state of network operations.

### API lists

| Function                    | Description                                                   |
|----------------------------|---------------------------------------------------------------|
| `SendDataSyncMeta`         | Sends post-processing result synchronously.                   |
| `DataExportAwait`          | Waits for the completion of an asynchronous operation.  <br>Currently, only `-1` can be specified for the timeout parameter; other values will be replaced. |
| `DataExportCleanup`        | Cleans up resources associated with the provided future.      |
| `DataExportSendData`       | Sends serialized data asynchronously.                         |
| `DataExportSendState`      | Sends state data asynchronously.                              |
| `DataExportStopSelf`       | Notifies the state machine to transition to 'Idle' state.     |
| `DataExportIsEnabled`      | Checks whether sending data of the specified type is enabled. |
| `DataExportGetPortSettings`| Gets the current port settings in a JSON object.              |


## Usage Example

### Send input tensor

The following C++ code snippet demonstrates a basic usage scenario of the EdgeAppLib Data Export. It shows how to send data asynchronously, wait for the operation to complete, and then clean up resources. This example assumes that the EdgeAppLib Data Export system has been properly initialized and configured.


```cpp
int onIterate() {
    // Get channel from sensor
    EdgeAppLibSensorChannel channel = 0;
    int32_t ret = SensorFrameGetChannelFromChannelId(
        *frame, CHANNEL_ID_INFERENCE_INPUT_IMAGE, &channel);

    // Get input tensor from sensor
    struct EdgeAppLibSensorRawData rawdata = {0};
    ret = SensorChannelGetRawData(channel, &rawdata);

    // Initiating an asynchronous send operation
    char *portname = "input";
    EdgeAppLibDataExportFuture *future = 
        DataExportSendData(portname, EdgeAppLibDataExportRaw,
                           rawdata.address, rawdata.size, rawdata.timestamp);

    // Optional: Perform additional tasks or computations here
    DataExportAwait(future, -1);

    // Cleaning up the resources associated with the future
    res = DataExportCleanup(future);
    assert(res == EdgeAppLibDataExportResultSuccess);
}
```

> **NOTE**
> 
> After `SensorChannelGetRawData` API for getting input tensor is called,  Data Export APIs (`DataExportSendData`, `DataExportCleanup`, Optional: `DataExportAwait`) might also be called.

### Send post-processing result

The following C++ code snippet demonstrates a basic usage scenario of the EdgeAppLib Data Export `SendDataSyncMeta`. The API wraps some operations related to `DataExportSendData`.

```cpp
int onIterate() {
    // Get channel from sensor
    EdgeAppLibSensorChannel channel = 0;
    int32_t ret = SensorFrameGetChannelFromChannelId(
        *frame, CHANNEL_ID_INFERENCE_OUTPUT, &channel);

    // Get metadata from sensor
    struct EdgeAppLibSensorRawData rawdata = {0};
    int32_t ret = SensorChannelGetRawData(channel, &rawdata);

    // Duplicate strings for sending data
    char *data = strdup("{\"mykey\":\"myvalue\"}");

    // Send by json text format
    EdgeAppLibSendDataResult result =
        SendDataSyncMeta(data, strlen(data), EdgeAppLibSendDataJson,
                         rawdata.timestamp, 10000);
    assert(result == EdgeAppLibSendDataResultSuccess ||
           result == EdgeAppLibSendDataResultEnqueued);
}
```

## API for Receive data
The Data Receive provides an interface for dowloading data from HTTP or Azure BlobStorage as a file. e.g. AI model.

### API lists

| Function                    | Description                                                  |
|-------------------------------------|------------------------------------------------------|
| `EdgeAppLibReceiveData`             | Receive data files from a remote location            |
| `EdgeAppLibReceiveDataStorePath`    | Get the file path after downloading                  |
|                                                                                            |

## Usage Example

```cpp
#include "receive_data.h"

#define MODEL_URL "http://0.0.0.0:8000/LPR.tflite"
#define DOWNLOAD_FILENAME "lp_recognition"

int onStart() {
  EdgeAppLibReceiveDataInfo info;
  info.filename = strdup(DOWNLOAD_FILENAME);
  info.filenamelen = strlen(DOWNLOAD_FILENAME);
  info.url = strdup(MODEL_URL);
  info.urllen = strlen(MODEL_URL);
  info.hash = strdup("6fb13ba628bd4dbf168c33f7099727f6cb21420be4fd32cdc8b319f2d0d736cf");
  EdgeAppLibReceiveDataResult ret2 = EdgeAppLibReceiveData(&info, -1);
  if (ret2 != EdgeAppLibReceiveDataResultSuccess) {
    LOG_ERR(
        "EdgeAppLibReceiveData failed with EdgeAppLibReceiveDataResult: %d",
        ret2);
    return -1;
  }
  LOG_INFO("EdgeAppLibReceiveData, download AI model from %s to local %s\n",
           info.url, info.filename);

  const char *path = EdgeAppLibReceiveDataStorePath();
  LOG_INFO("lpr_ai_model is downloaded in: %s/%s.%s", path, DOWNLOAD_FILENAME, "tflite");

  free(info.filename);
  free(info.url);
  free(info.hash);
}
```
