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
|----------------------------|---------------------------------------------------------------|
| `EdgeAppLibReceiveData`    | Receive data files from a remote location                     |
|                                                                                            |

## Usage Example

```cpp
#include "receive_data.h"

#define MODEL_URL "http://0.0.0.0:8000/network.pkg"
#define DOWNLOAD_FILENAME "./network.pkg"

int onStart() {
  EdgeAppLibReceiveDataInfo info;
  info.filename = strdup(DOWNLOAD_FILENAME);
  info.filenamelen = strlen(DOWNLOAD_FILENAME);
  info.url = strdup(MODEL_URL);
  info.urllen = strlen(MODEL_URL);
  EdgeAppLibReceiveDataResult ret2 = EdgeAppLibReceiveData(&info, -1);
  if (ret2 != EdgeAppLibReceiveDataResultSuccess) {
    LOG_ERR(
        "EdgeAppLibReceiveData failed with EdgeAppLibReceiveDataResult: %d",
        ret2);
    return -1;
  }
  LOG_INFO("EdgeAppLibReceiveData, download AI model from %s to local %s\n",
           info.url, info.filename);
  free(info.filename);
  free(info.url);
}
```
