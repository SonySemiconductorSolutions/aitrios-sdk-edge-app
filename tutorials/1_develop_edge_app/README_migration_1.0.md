# "**Edge Application**" Migration Guide from SDK v1.0

If you have already developed the "**Edge Application**" using [SDK v1.0](https://github.com/SonySemiconductorSolutions/aitrios-sdk-vision-sensing-app),
you need to modify the source code of the "**Edge Application**" to migrate to this SDK.

## Overview

In SDK v1.0, the "**Edge Application**" can:

- Customize behavior by changing parameters using **`PPLParameter`** in Command Parameter File of StartUploadInferenceData
- Receive and analyze IMX500 Output Tensor data
- Upload the analyze Metadata of inference result
- Customize a whole behavior, such as sequence and timing of getting Output Tensor, analyzing and uploading by custom implementation in event loop

Additionally, in this SDK, the "**Edge Application**" can:
- Control the upload of the Input Tensor data from the "**Edge Application**"

While in SDK a developer can as well customize the application behavior and configure the workflow, large portion of the communication logic is now encapsulated in the internal state machine implementation, leaving a user with a task of implementing the processing steps inside the event functions. 

Regarding the parameters take into account the following modifications: 
-  **`PPLParameters`** are replaced with the Custom Parameters in the DTDL configurations
- **`NumberOfImages`** is replaced by **`number_of_iterations`** in the [DTDL configuration](edge_app_interface.json#L244) 
- **`NumberOfInferencesPerMessage`** is replaced by **`number_of_inference_per_message`** in the [DTDL configuration](edge_app_interface.json#L787) 
- **`UploadInterval`** has been removed. Frequency of upload for inference and images now will correspond to the calls of **`DataExportSendData()`** that depends on the developers implementation. Upload interval cannot be adjusted anymore by the parameters defined under common_settings in DTDL

## Sequence of SDK v1.0

Wasm has a main loop and a thread. Wasm functions are called from Wasm and Native callback.

> **NOTE**
>
> This sequence is based on the processing of the sample object detection code. The set of called methods varies depending on the logic of the sample app.

<!-- mermaid alt text: Sequence of SDK v1.0 -->
```mermaid
sequenceDiagram
participant Native as Native (Firmware)
participant Wasm_Lib as Wasm Native Library (Firmware)
participant Wasm as Wasm (Edge Application)
participant Wasm_Thread as Wasm thread (Edge Application)
Native->>Wasm: Main()
Wasm->>Wasm_Thread: pthread_create
Wasm_Thread-->>Wasm: 
Wasm->>Wasm_Lib: SessInit()
Wasm_Lib-->>Wasm: 
Wasm->>Wasm_Lib: SessRegisterSendDataCallback()
Wasm_Lib-->>Wasm: 
Wasm_Thread->>Wasm_Lib: EVP_setConfigurationCallback()
Wasm_Lib-->>Wasm_Thread: 
loop Loop in Wasm thread
    Wasm_Thread->>Wasm_Lib: EVP_processEvent()
    Wasm_Lib-->>Wasm_Thread: 
    alt Receive PPL Parameter
    Native->>Native: Receive PPL Parameter from Console
    Native->>Wasm_Lib: Set Configuration
    Wasm_Lib->>Wasm_Thread: ConfigurationCallback
    Wasm_Thread->>Wasm_Thread: Get PPL Parameter
    Wasm_Thread-->> Wasm_Lib: 
    Wasm_Lib-->>Native: 
    end
end
loop Loop in Main
    Wasm->>Wasm_Lib: senscord_channel_get_raw_data()
    Wasm_Lib-->>Wasm: 
    Wasm->>Wasm: Analyze and Serialize
    Wasm->>Wasm_Lib: SessMalloc()
    Wasm_Lib-->>Wasm: 
    Wasm->>Wasm_Lib: SessSendData()
    Wasm_Lib->>Native: 
    Native->>Native: Send to Console
    Native-->>Wasm_Lib: 
    Wasm_Lib-->>Wasm: 
    Wasm_Lib->>Wasm: SendDataDoneCallback
    Wasm->>Wasm_Lib: SessFree()
    Wasm_Lib-->>Wasm: 
    Wasm-->>Wasm_Lib: 
end
Wasm->>Wasm_Thread: pthread_join
Wasm_Thread-->>Wasm: 
Wasm->>Wasm_Lib: SessExit()
Wasm_Lib-->>Wasm: 
Wasm-->>Native: 
```

## Sequence of this SDK

> **NOTE**
>
> This sequence is based on the processing of the [sample object detection](../../sample_apps/detection) code.

<!-- mermaid alt text: Sequence of SDK v1.0 -->
```mermaid
sequenceDiagram
    participant EVP Cloud
    participant EVP Agent
    participant Wasm Native Library
    participant State Machine
    participant WasmThread (State Machine)
    participant Event Functions

%% to Creating State
    EVP Cloud->>EVP Agent: Deployment Manifest
    EVP Agent->>State Machine: Instantiate
rect rgba(0,0,255,0.1) 
note over Wasm Native Library, Event Functions: Creating state
    State Machine->>State Machine: creating = Creating()
    State Machine->>State Machine: context = GetInstance(creating)
    State Machine->>Wasm Native Library: evp_client = EVP_initialize()
    State Machine->>State Machine: Iterate()
    State Machine->>Wasm Native Library: EVP_setConfigurationCallback()
    State Machine->>Wasm Native Library: EVP_setRpcCallback()
    State Machine->>Wasm Native Library: EVP_setMessageCallback()
    State Machine->>Event Functions: onCreate()
    State Machine->>State Machine: SetNextState(STATE_IDLE)
    State Machine-->>State Machine: return
end

%% to Idle State
rect rgba(0,0,255,0.1)
note over EVP Cloud, Event Functions: Idle state
    State Machine->>State Machine: GetNextState()
    State Machine->>State Machine: Instantiate + Iterate()
    loop
        State Machine->>Wasm Native Library: EVP_processEvent()
    end

    EVP Cloud->>EVP Agent: Instance State Machine<br>Update `processState` to `running`
    EVP Agent->>Wasm Native Library: Add Event to queue
    State Machine->>Wasm Native Library: EVP_processEvent()
    Wasm Native Library->>State Machine: SetNextState(STATE_RUNNING)
    Wasm Native Library-->>State Machine: evp_configuration_callback()
    State Machine-->>State Machine: return
end

%% to Running State
rect rgba(0,0,255,0.1)
note over EVP Cloud, Event Functions: Running state
    State Machine->>State Machine: GetNextState()
    State Machine->>State Machine: Instantiate + Iterate()
    State Machine->>WasmThread (State Machine): Spawn new thread
    loop
        WasmThread (State Machine)->>Event Functions: onIterate()
    end

    loop
        State Machine->>Wasm Native Library: EVP_processEvent()
    end

    EVP Cloud->>EVP Agent: Instance State Machine<br>Update parameter from DTDL
    EVP Agent->>Wasm Native Library: Add Event to queue

    State Machine->>Wasm Native Library: EVP_processEvent()

    alt If configuration in developer DTDL
        Wasm Native Library->>Event Functions: onConfigure(topic, value, valuelen)
    else If configuration in PQ DTDL
        Wasm Native Library->> Wasm Native Library: Configure(topic, value, valuelen)
    else if configuration in SM DTDL
        Wasm Native Library->>State Machine: SM_Configure(topic, value, valuelen)
    end

    Wasm Native Library-->>State Machine: evp_configuration_callback()

    loop
        State Machine->>Wasm Native Library: EVP_processEvent()
    end
end

%% to Destroying State
    EVP Cloud->>EVP Agent: Deployment Manifest
    EVP Agent->>Wasm Native Library: Update desired Deployment Manifest
rect rgba(0,0,255,0.1)
note over Wasm Native Library, Event Functions: Deleting state
    State Machine->>Wasm Native Library: EVP_processEvent()
    Wasm Native Library-->>State Machine: EVP_SHOULDEXIT
    State Machine->>State Machine: SetNextState(STATE_DESTROYING)
    State Machine->>WasmThread (State Machine): Kill thread
    State Machine->>Event Functions: onStop()
    State Machine-->>State Machine: return
    Note over State Machine: Gracefully exiting
end

%%
rect rgba(0,0,255,0.1)
note over Wasm Native Library, Event Functions: Destroying state
    State Machine->>State Machine: Instantiate + Iterate()
    State Machine->>Event Functions: onDestroy()
    Event Functions-->>State Machine: return
    State Machine->>State Machine: SetNextState(STATE_EXITING)
    State Machine-->>State Machine: return
end
```


## Migration steps

The sample code of "**Edge Application**" in SDK v1.7 is implemented to reproduce behavior of such in SDK v1.0. We further illustrate the migration steps using the object detection example in the SDK v1.0 [sample applications](https://github.com/SonySemiconductorSolutions/aitrios-sdk-vision-sensing-app/tree/v1.0.6/tutorials/4_prepare_application/1_develop/sdk/sample) and detection "**Edge Application**" in SDK v1.7 [sample application](../../sample_apps).

> **Note** <br>
> We provide a minor change related to versioning in the header. In particular, the previously present check of the id and version of the header is eliminated. It is not related to the internal logic of the SDK.


1. Place your FlatBuffers `.fbs` file in the [`schemas` folder](../../sample_apps/detection/schemas)

2. Regenerate FlatBuffers header file. <br>
   Please see [ Output serialization with a FlatBuffers schema](README.md#2-output-serialization-with-a-flatbuffers-schema). Specify `<your-output-folder-path>`, the folder path to the generated header file, to be [include/schemas](../../sample_apps/detection/include/schemas/) of your app.

3. Migrate to use AitriosDataExport API.<br>
   Replace the [following logic from SDK v1.0](https://github.com/SonySemiconductorSolutions/aitrios-sdk-vision-sensing-app/blob/v1.0.6/tutorials/4_prepare_application/1_develop/sdk/vision_app_sdk/include/vision_app_public.h#L191-L204) with the one provided in AitriosDataExport functions from the [`data_export.h`](../../include/data_export.h).

    <details>
    <summary>Public data export functions.</summary>

    | SDK v1.0                         | SDK v2.0                                      |
    |----------------------------------|-----------------------------------------------|
    | `SessInit`                       | Not available                                 |
    | `SessExit`                       | Not available                                 |
    | `SessSendData`                   | `DataExportSendData`                   |
    | `SessMalloc`                     | Not available, use `malloc` from `<stdlib.h>` |
    | `SessFree`                       | `DataExportCleanup`                    |
    | `SessRegisterSendDataCallback`   | Not available                                 |
    | `SessUnregisterSendDataCallback` | Not available                                 |

    SDK v2.0 introduced the following new functions:

    | Function                     | description                                                      |
    |------------------------------|------------------------------------------------------------------|
    | `DataExportSendState` | Send state asynchronously.                                       |
    | `DataExportAwait`     | Waits for the completion of an asynchronous operation.           |
    | `DataExportStopSelf`  | notifies state machine to transition from Running to Idle state. |

    </details>
    <br>

4. Migrate to use AitriosSensor API instead of Senscord from `vision_app_public.h`.<br>
    Introduce AitriosSensor functions from the [`sensor.h`](../../include/sensor.h) instead of their Senscord contraparts in SDK v1.0 [example](https://github.com/SonySemiconductorSolutions/aitrios-sdk-vision-sensing-app/blob/v1.0.6/tutorials/4_prepare_application/1_develop/sdk/vision_app_sdk/include/vision_app_public.h#L139-L189).

    <details>
    <summary>Public senscord functions.</summary>

    | SDK v1.0                                     | SDK v2.0                                     |
    |----------------------------------------------|----------------------------------------------|
    | `senscord_core_init`                         | `AitriosSensorCoreInit`                      |
    | `senscord_core_exit`                         | `AitriosSensorCoreExit`                      |
    | `senscord_core_open_stream`                  | `AitriosSensorCoreOpenStream`                |
    | `senscord_core_close_stream`                 | `AitriosSensorCoreCloseStream`               |
    | `senscord_stream_start`                      | `AitriosSensorStart`                         |
    | `senscord_stream_stop`                       | `AitriosSensorStop`                          |
    | `senscord_stream_get_frame`                  | `AitriosSensorGetFrame`                      |
    | `senscord_stream_release_frame`              | `AitriosSensorReleaseFrame`                  |
    | `senscord_stream_get_property`               | `AitriosSensorStreamGetProperty`             |
    | `senscord_stream_set_property`               | `AitriosSensorStreamSetProperty`             |
    | `senscord_stream_register_frame_callback`    | `AitriosSensorStreamRegisterFrameCallback`   |
    | `senscord_stream_unregister_frame_callback`  | `AitriosSensorStreamUnregisterFrameCallback` |
    | `senscord_frame_get_channel_from_channel_id` | `AitriosSensorFrameGetChannelFromChannelId`  |
    | `senscord_channel_get_raw_data`              | `AitriosSensorChannelGetRawData`             |
    | `senscord_channel_get_property`              | `AitriosSensorChannelGetProperty`            |
    | `senscord_get_last_error` -> level           | `AitriosSensorGetLastErrorLevel`             |
    | `senscord_get_last_error` -> cause           | `AitriosSensorGetLastErrorCause`             |
    | `senscord_get_last_error` -> message         | `AitriosSensorGetLastErrorString`            |

    </details>
    <br>

5. Split the logic between event functions.<br>
    In the SDK v1.0 the entrypoint of the application is the **`main()`** function provided by a developer. In the SDK v1.7 the workflow is represented by the state machine calling the event functions declared in [**`sm.h`**](../../include/sm.h). Thus, to shift to the newer version, the developer then has to split the [original code](https://github.com/SonySemiconductorSolutions/aitrios-sdk-vision-sensing-app/blob/v1.0.6/tutorials/4_prepare_application/1_develop/sdk/sample/vision_app/single_dnn/objectdetection/src/vision_app_objectdetection.cpp) into [the elements](../../sample_apps/detection/src/sm.cpp) that belong to the event functions. The table below provides refences to the code snippets.

    <details>
    <summary>Correspondence between the lines of the original code and the event functions.</summary>

    | SDK v1.7      | SDK v1.0 |  Used functionality |
    |---------------|----------|---------------------|
    | [`onCreate()`](../../sample_apps/detection/src/sm.cpp#L139-L161)  | [L112-L133](https://github.com/SonySemiconductorSolutions/aitrios-sdk-vision-sensing-app/blob/v1.0.6/tutorials/4_prepare_application/1_develop/sdk/sample/vision_app/single_dnn/objectdetection/src/vision_app_objectdetection.cpp#L112-L133) of `main()` | `AitriosSensorCoreInit`,<br>`AitriosSensorCoreOpenStream`,<br>`AitriosSensorStreamRegisterFrameCallback` |
    | [`onStart()`](../../sample_apps/detection/src/sm.cpp#L227-L246) | [L126-L151](https://github.com/SonySemiconductorSolutions/aitrios-sdk-vision-sensing-app/blob/v1.0.6/tutorials/4_prepare_application/1_develop/sdk/sample/vision_app/single_dnn/objectdetection/src/vision_app_objectdetection.cpp#L126-L151) of `main()` |   `AitriosSensorStart`,<br>`AitriosSensorImageCropProperty`,<br>`AitriosSensorStreamGetProperty` | 
    | [`onIterate()`](../../sample_apps/detection/src/sm.cpp#L188-L214) | [L162-L234](https://github.com/SonySemiconductorSolutions/aitrios-sdk-vision-sensing-app/blob/v1.0.6/tutorials/4_prepare_application/1_develop/sdk/sample/vision_app/single_dnn/objectdetection/src/vision_app_objectdetection.cpp#L162-L234) of `main()` |  `AitriosSensorGetFrame`,<br>`AitriosSensorFrameGetChannelFromChannelId`,<br>`AitriosSensorChannelGetRawData`,<br>`AitriosDataExportSendData`,<br>`AitriosDataExportAwait`,<br>`AitriosDataExportCleanup`,<br>`AitriosSensorReleaseFrame`<br>`DataProcessorAnalyze` |
    | [`onStop()`](../../sample_apps/detection/src/sm.cpp#L216-L225) | [L244-L255](https://github.com/SonySemiconductorSolutions/aitrios-sdk-vision-sensing-app/blob/v1.0.6/tutorials/4_prepare_application/1_develop/sdk/sample/vision_app/single_dnn/objectdetection/src/vision_app_objectdetection.cpp#L244-L255) of `main()` |   `AitriosSensorStop` |
    | [`onDestroy()`](../../sample_apps/detection/src/sm.cpp#L248-L267) | [L236-L269](https://github.com/SonySemiconductorSolutions/aitrios-sdk-vision-sensing-app/blob/v1.0.6/tutorials/4_prepare_application/1_develop/sdk/sample/vision_app/single_dnn/objectdetection/src/vision_app_objectdetection.cpp#L236-L269) of `main()` | `AitriosSensorStreamUnregisterFrameCallback`,<br>`AitriosSensorCoreCloseStream`,<br> `AitriosSensorCoreExit` |
    | [`onConfigure()`](../../sample_apps/detection/src/sm.cpp#L163-L186) | [`ConfigurationCallback()`](https://github.com/SonySemiconductorSolutions/aitrios-sdk-vision-sensing-app/blob/v1.0.6/tutorials/4_prepare_application/1_develop/sdk/sample/vision_app/single_dnn/objectdetection/src/vision_app_objectdetection.cpp#L348-L402) | `DataProcessorConfigure` (parse the custom parameter json and sets the configuration parameters),<br>`DataExportSendState` (updates the custom parameters in DTDL) |

    </details>
    <br>

6. Build "**Edge Application**".<br>
   See [Build the "**Edge Application**"](README.md#build-the-edge-application)


