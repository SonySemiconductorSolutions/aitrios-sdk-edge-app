# Building the Tiny Vest Counter Edge App: A Beginner's Guide

This guide walks you through creating a new Edge Application called `tiny_vest_counter` that uses a Tiny AI model for vest detection.

## Why Choose the Classification Sample App?

After reviewing all sample applications in this repository, **the `classification` sample app is the best starting point** for your Tiny Vest Counter project. Here's why:

| Factor | Classification | Detection | Other Samples |
|--------|---------------|-----------|---------------|
| **Output Format** | Class scores (exactly what you need for 2-class vest detection) | Bounding boxes + class IDs (more complex) | Various specialized formats |
| **Complexity** | Simple score processing | Complex coordinate handling | Variable |
| **Learning Curve** | Beginner-friendly | Intermediate | Variable |
| **Modification Needed** | Minimal - add threshold + counter | Significant restructuring | Major changes |

The `classification` sample already:
- Receives inference results (float scores) from IMX500
- Processes multiple class scores
- Sends metadata to Local Console
- Has a simple, understandable code structure

You only need to add:
- A threshold check for vest detection
- A counter that increments on detection
- Custom metadata fields (timestamp, vest_detected, vest_count, score)
- Console logging

---

## Step-by-Step Development Guide

The following table breaks down all the work needed to build your `tiny_vest_counter` Edge App. Time estimates assume you are a beginner using GitHub Copilot for assistance.

| Step | Task Name | Description | Estimated Time |
|------|-----------|-------------|----------------|
| **Step 1** | **Understand the Repository** | Clone the repository, explore the folder structure (`sample_apps/`, `libs/`, `include/`), and read the main `README.md` to understand how Edge Apps work. | 1-2 hours |
| **Step 2** | **Set Up Development Environment** | Install Docker (for building), set up your IDE, and run `git submodule update --init --recursive` to get all dependencies. | 1-2 hours |
| **Step 3** | **Build & Test Classification Sample** | Build the classification sample with `make CMAKE_FLAGS="-DAPPS_SELECTION=classification"` and verify it compiles successfully. This confirms your environment works. | 30 min - 1 hour |
| **Step 4** | **Study the Classification Sample Code** | Read through `sample_apps/classification/src/sm.cpp` and `data_processor/src/classification_data_processor.cpp` to understand how inference results are received and processed. | 1-2 hours |
| **Step 5** | **Create the New App Folder** | Copy the `classification` folder to create `sample_apps/tiny_vest_counter/`. Rename internal files appropriately (e.g., `classification_utils.cpp` → `vest_counter_utils.cpp`). | 30 min - 1 hour |
| **Step 6** | **Understand Tiny Model Output** | Learn that your Tiny AI model outputs 2 float scores: one for "vest" and one for "no vest". The higher score indicates the classification result. | 30 min - 1 hour |
| **Step 7** | **Add Threshold Parameter** | Modify the configuration to accept a `threshold` parameter (default: 0.7). Update `vest_counter_utils.hpp` to include this in `DataProcessorCustomParam`. | 1-2 hours |
| **Step 8** | **Implement Counter Logic** | Add a static counter variable that increments when the vest score exceeds the threshold. Reset logic is optional based on your needs. | 1-2 hours |
| **Step 9** | **Define Metadata Structure** | Create a new FlatBuffers schema (`.fbs`) or JSON structure with fields: `timestamp`, `vest_detected` (bool), `vest_count` (int), `score` (float). | 1-2 hours |
| **Step 10** | **Compile FlatBuffers Schema** | If using FlatBuffers, run `tools/compile_fbs.sh vest_counter.fbs include/schemas/` to generate the C++ header. | 30 min |
| **Step 11** | **Implement Metadata Creation** | Modify `vest_counter_data_processor.cpp` to create metadata with your custom fields instead of classification list. | 1-2 hours |
| **Step 12** | **Add Console Logging** | Add `LOG_INFO` statements to output a one-line summary: `"[VestCounter] vest_detected=true, count=5, score=0.85"`. | 30 min - 1 hour |
| **Step 13** | **Update CMakeLists.txt** | Create `sample_apps/tiny_vest_counter/CMakeLists.txt` and `data_processor/src/CMakeLists.txt` with correct file paths and dependencies. | 30 min - 1 hour |
| **Step 14** | **Update Main CMakeLists.txt** | Add `tiny_vest_counter` to the `APPS_LIST` in the root `CMakeLists.txt` so the build system recognizes your app. | 15 min |
| **Step 15** | **Create Configuration Files** | Create `configuration/configuration.json` with your app's parameters (threshold, process_state, etc.). | 30 min - 1 hour |
| **Step 16** | **Create Package Files** | Create `package/manifest.json` and `package/edge_app_vc_interface.json` (DTDL) for deployment. | 30 min - 1 hour |
| **Step 17** | **Build Your App** | Build with `make CMAKE_FLAGS="-DAPPS_SELECTION=tiny_vest_counter"` and fix any compilation errors. | 1-2 hours |
| **Step 18** | **Test with Mock Data** | Create test data files and run unit tests to verify your logic works correctly. | 1-2 hours |
| **Step 19** | **Test on Raspberry Pi** | Deploy to Raspberry Pi, connect to IMX500 camera with your Tiny AI model, and verify end-to-end functionality. | 2-3 hours |
| **Step 20** | **Write README** | Document your app: what it does, how to configure it, expected outputs, and usage examples. | 1-2 hours |

---

## Total Estimated Time

| Phase | Steps | Time Range |
|-------|-------|------------|
| **Setup & Learning** | 1-4 | 3.5 - 6 hours |
| **Core Development** | 5-16 | 8 - 15 hours |
| **Build & Test** | 17-19 | 4 - 7 hours |
| **Documentation** | 20 | 1 - 2 hours |
| **Total** | 1-20 | **16.5 - 30 hours** |

With Copilot assistance and following this guide, a beginner can complete this project in approximately **2-4 working days**.

---

## Key Files to Modify

Based on the classification sample, here are the main files you'll work with:

```
sample_apps/tiny_vest_counter/
├── CMakeLists.txt                          # Build configuration
├── README.md                               # Your app documentation
├── configuration/
│   └── configuration.json                  # Runtime configuration
├── data_processor/
│   ├── src/
│   │   ├── CMakeLists.txt                  # Data processor build config
│   │   ├── vest_counter_data_processor.cpp # Main processing logic ★
│   │   ├── vest_counter_utils.cpp          # Helper functions
│   │   └── vest_counter_utils.hpp          # Structs and declarations
│   └── tests/                              # Unit tests
├── include/
│   └── schemas/
│       └── vest_counter_generated.h        # Generated from .fbs
├── package/
│   ├── edge_app_vc_interface.json          # DTDL interface
│   └── manifest.json                       # Package manifest
├── schemas/
│   └── vest_counter.fbs                    # FlatBuffers schema
└── src/
    └── sm.cpp                              # State machine (minimal changes)
```

---

## Example: Core Counter Logic

Here's a simplified example of what your vest detection logic might look like:

```cpp
// In vest_counter_data_processor.cpp

static uint32_t vest_count = 0;  // Persistent counter

DataProcessorResultCode DataProcessorAnalyze(float *in_data, uint32_t in_size,
                                             char **out_data, uint32_t *out_size) {
    // Assuming index 0 = "vest" score, index 1 = "no vest" score
    float vest_score = in_data[0];
    float threshold = params.threshold;  // e.g., 0.7
    
    bool vest_detected = (vest_score > threshold);
    
    if (vest_detected) {
        vest_count++;
    }
    
    // Log to console
    LOG_INFO("[VestCounter] vest_detected=%s, count=%u, score=%.2f",
             vest_detected ? "true" : "false", vest_count, vest_score);
    
    // Create and return metadata (FlatBuffers or JSON)
    // ...
}
```

---

## Example: FlatBuffers Schema

```flatbuffers
// vest_counter.fbs
namespace VestCounter;

table VestDetectionResult {
  timestamp:uint64;      // Unix timestamp
  vest_detected:bool;    // True if vest score > threshold
  vest_count:uint32;     // Accumulated detection count
  score:float;           // Current vest score
}

table VestCounterTop {
  result:VestDetectionResult;
}

root_type VestCounterTop;
```

---

## Tips for Beginners

1. **Start Small**: First get the app to compile, then add features one at a time.

2. **Use Copilot**: When stuck on syntax or patterns, describe what you want in comments and let Copilot suggest code.

3. **Check Logs**: Use `LOG_INFO`, `LOG_DBG`, `LOG_ERR` liberally to debug issues.

4. **Test Incrementally**: Build and test after each significant change.

5. **Compare with Sample**: When unsure, look at how the classification sample does something similar.

6. **Keep It Simple**: Since you're using a Tiny AI model, avoid adding unnecessary complexity. The simpler your code, the easier it is to debug and maintain.

---

## Next Steps

Once you complete this guide:

1. **Deploy to Device**: Follow the [Import Edge App tutorial](../../tutorials/2_import_edge_app/README.md) to package and deploy.

2. **Connect Local Console**: Set up the Local Console to visualize your vest detection results.

3. **Iterate**: Adjust threshold values and test with real camera data to optimize accuracy.

Good luck building your Tiny Vest Counter Edge App! 🦺
