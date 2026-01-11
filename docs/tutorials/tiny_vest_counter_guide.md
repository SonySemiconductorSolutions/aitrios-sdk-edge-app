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

## 🚀 Time-Saving Improvements

Before diving into the steps, here are key strategies to **reduce development time by up to 50%**:

| Improvement | Original Approach | Optimized Approach | Time Saved |
|-------------|-------------------|-------------------|------------|
| **Use JSON metadata** | FlatBuffers schema + compilation | Direct JSON output (already supported) | ~1.5 hours |
| **Reuse sm.cpp** | Modify state machine code | Use classification's sm.cpp as-is | ~1 hour |
| **Script-based setup** | Manual file copy/rename | Use `cp -r` + `sed` commands | ~30 min |
| **Skip unit tests initially** | Write tests before deployment | Test on device first, add tests later | ~1.5 hours |
| **Use sample configs as templates** | Create from scratch | Copy and modify existing files | ~30 min |
| **Combine similar steps** | 20 separate steps | 12 consolidated steps | ~1 hour |

### Quick-Start Commands

```bash
# 1. Clone and set up (Step 1-2 combined)
git clone --recursive https://github.com/SonySemiconductorSolutions/aitrios-sdk-edge-app.git
cd aitrios-sdk-edge-app

# 2. Create app from template (Step 5 automated)
cp -r sample_apps/classification sample_apps/tiny_vest_counter
cd sample_apps/tiny_vest_counter/data_processor/src
# Rename files (works on all Linux/macOS)
for f in classification_*; do mv "$f" "${f/classification_/vest_counter_}"; done

# 3. Build (after modifications)
cd ../../../..
make CMAKE_FLAGS="-DAPPS_SELECTION=tiny_vest_counter"
```

---

## Step-by-Step Development Guide (Optimized)

The following table uses an **optimized workflow** that consolidates steps and leverages existing code. Time estimates assume you are a beginner using GitHub Copilot for assistance.

| Step | Task Name | Description | Estimated Time |
|------|-----------|-------------|----------------|
| **Step 1** | **Clone & Setup Environment** | Clone the repository with `--recursive` flag, install Docker, and build the classification sample to verify your environment: `make CMAKE_FLAGS="-DAPPS_SELECTION=classification"` | 1 - 1.5 hours |
| **Step 2** | **Study Sample Code** | Read through `sample_apps/classification/data_processor/src/classification_data_processor.cpp` focusing on `DataProcessorAnalyze()` function. Understand how scores are processed. | 30 min - 1 hour |
| **Step 3** | **Create App from Template** | Copy classification folder: `cp -r sample_apps/classification sample_apps/tiny_vest_counter`. Rename files using find/sed or manually rename `classification_*` files to `vest_counter_*`. | 15 - 30 min |
| **Step 4** | **Add Threshold & Counter Logic** | Modify `vest_counter_data_processor.cpp`: add static counter, threshold check, increment logic, and `LOG_INFO` for console output. | 1 - 1.5 hours |
| **Step 5** | **Switch to JSON Metadata** | Use `EdgeAppLibSendDataJson` format instead of FlatBuffers. Create JSON with parson library: `{"timestamp":..., "vest_detected":..., "vest_count":..., "score":...}`. No schema compilation needed! | 45 min - 1 hour |
| **Step 6** | **Update Configuration** | Copy and modify `configuration/configuration.json` to add `threshold` parameter and set `metadata_settings.format` to `1` (JSON). | 15 - 30 min |
| **Step 7** | **Update CMakeLists.txt** | Update paths in `data_processor/src/CMakeLists.txt` to point to your renamed files. The main CMakeLists.txt auto-discovers apps. | 15 - 30 min |
| **Step 8** | **Build & Fix Errors** | Build with `make CMAKE_FLAGS="-DAPPS_SELECTION=tiny_vest_counter"`. Fix any compilation errors (usually typos or missing includes). | 30 min - 1 hour |
| **Step 9** | **Test on Raspberry Pi** | Deploy to Raspberry Pi with your Tiny AI model. Use Local Console to view metadata output and verify counter increments correctly. | 1.5 - 2 hours |
| **Step 10** | **Create Package Files** | Create `package/manifest.json` and DTDL interface file for Console deployment. Copy from classification sample and modify app name. | 15 - 30 min |
| **Step 11** | **Write README** | Document your app: purpose, configuration parameters, expected outputs. Keep it brief - Copilot can help generate this quickly. | 30 min - 1 hour |
| **Step 12** | **Add Unit Tests (Optional)** | Once the app works, add tests for counter logic. Use existing test files as templates. This can be done post-deployment. | 1 - 1.5 hours (optional) |

---

## Total Estimated Time (Optimized)

| Phase | Steps | Time Range |
|-------|-------|------------|
| **Setup & Learning** | 1-2 | 1.5 - 2.5 hours |
| **Core Development** | 3-7 | 2.5 - 4.5 hours |
| **Build & Test** | 8-9 | 2 - 3 hours |
| **Packaging & Docs** | 10-11 | 45 min - 1.5 hours |
| **Optional Tests** | 12 | 1 - 1.5 hours |
| **Total (without optional)** | 1-11 | **6.75 - 11.5 hours** |
| **Total (with optional)** | 1-12 | **7.75 - 13 hours** |

With these optimizations and Copilot assistance, a beginner can complete this project in approximately **1-1.5 working days** (compared to 2-4 days with the original approach).

### Time Reduction Summary

| Approach | Estimated Time | Working Days |
|----------|---------------|--------------|
| **Original (20 steps)** | 16.5 - 30 hours | 2-4 days |
| **Optimized (12 steps)** | 6.75 - 11.5 hours | 1-1.5 days |
| **Time Saved** | ~10-18 hours | ~1-2.5 days |

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

## Example: Core Counter Logic with JSON Metadata (Recommended)

Here's a complete example using the **recommended JSON approach** (faster to implement):

```cpp
// In vest_counter_data_processor.cpp

#include "parson.h"
#include "log.h"
#include <sys/time.h>

static uint32_t vest_count = 0;  // Persistent counter

// Helper function to get timestamp in milliseconds
static uint64_t get_timestamp_ms() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)(tv.tv_sec) * 1000 + (uint64_t)(tv.tv_usec) / 1000;
}

DataProcessorResultCode DataProcessorAnalyze(float *in_data, uint32_t in_size,
                                             char **out_data, uint32_t *out_size) {
    // Assuming index 0 = "vest" score, index 1 = "no vest" score
    float vest_score = in_data[0];
    float threshold = params.threshold;  // e.g., 0.7
    
    bool vest_detected = (vest_score > threshold);
    
    if (vest_detected) {
        vest_count++;
    }
    
    // Log to console (one-line output for Raspberry Pi)
    LOG_INFO("[VestCounter] vest_detected=%s, count=%u, score=%.2f",
             vest_detected ? "true" : "false", vest_count, vest_score);
    
    // Create JSON metadata (simple and fast!)
    JSON_Value *root = json_value_init_object();
    JSON_Object *obj = json_value_get_object(root);
    json_object_set_number(obj, "timestamp", (double)get_timestamp_ms());
    json_object_set_boolean(obj, "vest_detected", vest_detected);
    json_object_set_number(obj, "vest_count", vest_count);
    json_object_set_number(obj, "score", vest_score);
    
    // Note: Caller (SendDataSyncMeta) is responsible for freeing out_data
    *out_data = json_serialize_to_string(root);
    *out_size = json_serialization_size(root);
    json_value_free(root);
    
    return kDataProcessorOk;
}

EdgeAppLibSendDataType DataProcessorGetDataType() { 
    return EdgeAppLibSendDataJson;  // Use JSON format
}
```

---

## Example: FlatBuffers Schema (Alternative)

If you prefer FlatBuffers (more efficient but requires schema compilation):

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

> **Note**: Using JSON saves ~1.5 hours by eliminating schema compilation and header generation steps.

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
