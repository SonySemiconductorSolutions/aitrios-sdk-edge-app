/****************************************************************************
 * Copyright 2024 Sony Semiconductor Solutions Corp. All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 ****************************************************************************/

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <string>

#include "log_internal.h"
#include "parson.h"
#include "switch_dnn_analyzer.h"

static bool g_flatbuffer_normal = true;

bool IsNormalFlatBuffer() { return g_flatbuffer_normal; }

static const float kOutputTensorOd[] = {
    // y_min
    0.1, 0.2, 0.3, 0.4, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    // x_min
    0.15, 0.25, 0.35, 0.45, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    // y_max
    0.5, 0.6, 0.7, 0.8, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    // x_max
    0.55, 0.65, 0.75, 0.85, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    // class
    15, 132, 15, 15, 0, 0, 0, 0, 0, 0,
    // score
    0.8, 0.2, 0.8, 0.3, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0,
    // num of detection
    10};

static const float kOutputTensorIc[] = {
    // score
    0.10, 0.81, 0.32, 0.63, 0.54};

class AnalyzerTest : public ::testing::Test {
 public:
  static void SetUpTestCase() { SetLogLevel(kDebugLevel); }
  static void TearDownTestCase() {}

 protected:
  void SetUp() override {
    common_ = new AnalyzerCommon();
    od_ = new AnalyzerOd();
    ic_ = new AnalyzerIc();
    InitParam();
    g_flatbuffer_normal = true;
  }
  void TearDown() override {
    delete common_;
    delete od_;
    delete ic_;
  }

  void InitParam();
  void ClearParam();

  class Allocator : public AnalyzerBase::Allocator {
   public:
    void *Malloc(size_t size) const { return malloc(size); }
    void Free(void *ptr) const { free(ptr); }
  };

  class InvalidAllocator : public AnalyzerBase::Allocator {
   public:
    void *Malloc(size_t size) const { return nullptr; }
    void Free(void *ptr) const { free(ptr); }
  };

  AnalyzerCommon *common_;
  AnalyzerOd *od_;
  AnalyzerIc *ic_;
  Allocator allocator_;
  InvalidAllocator invalid_allocator_;
};

void AnalyzerTest::InitParam() {
  std::string text = R"({
    "ai_models": {
      "detection_bird": {
        "ai_model_bundle_id": "000001",
        "param": {
          "max_detections": 3,
          "threshold": 0.3,
          "input_width": 300,
          "input_height": 300
        }
      },
      "classification_bird": {
        "ai_model_bundle_id": "000002",
        "param": {
          "max_predictions": 3
        }
      }
    }
  })";
  od_->ClearValidatingParam();
  od_->ValidateParam(text.c_str());
  od_->SetValidatedParam(text.c_str());
  ic_->ClearValidatingParam();
  ic_->ValidateParam(text.c_str());
  ic_->SetValidatedParam(text.c_str());
}

TEST_F(AnalyzerTest, ValidateParamOnCommon) {
  AnalyzerBase::ResultCode result = common_->ValidateParam("");
  EXPECT_EQ(AnalyzerBase::ResultCode::kOk, result);
}

TEST_F(AnalyzerTest, SetValidatedParamOnCommon) {
  AnalyzerBase::ResultCode result = common_->SetValidatedParam("");
  EXPECT_EQ(AnalyzerBase::ResultCode::kOk, result);
}

TEST_F(AnalyzerTest, ClearValidatingParamOnCommon) {
  AnalyzerBase::ResultCode result = common_->ClearValidatingParam();
  EXPECT_EQ(AnalyzerBase::ResultCode::kOk, result);
}

TEST_F(AnalyzerTest, AnalyzeOnCommon) {
  AnalyzerBase::ResultCode result = common_->Analyze(nullptr, 0, 0);
  EXPECT_EQ(AnalyzerBase::ResultCode::kInvalidParam, result);
}

TEST_F(AnalyzerTest, SerializeOnCommon) {
  AnalyzerBase::ResultCode result = common_->Serialize(nullptr, 0, nullptr);
  EXPECT_EQ(AnalyzerBase::ResultCode::kInvalidParam, result);
}

TEST_F(AnalyzerTest, GetNetworkIdOnCommon) {
  char network_id[AI_MODEL_BUNDLE_ID_SIZE] = {0};
  char expected_network_id[AI_MODEL_BUNDLE_ID_SIZE] = {0};
  AnalyzerBase::ResultCode result = common_->GetNetworkId(network_id);
  EXPECT_EQ(0, memcmp(network_id, expected_network_id, sizeof(network_id)));
  EXPECT_EQ(AnalyzerBase::ResultCode::kInvalidParam, result);
}

TEST_F(AnalyzerTest, ValidateParamSuccess) {
  std::string text = R"({
    "ai_models": {
      "detection_bird": {
        "ai_model_bundle_id": "000001",
        "param": {
          "max_detections": 3,
          "threshold": 0.3,
          "input_width": 300,
          "input_height": 300
        }
      },
      "classification_bird": {
        "ai_model_bundle_id": "000002",
        "param": {
          "max_predictions": 3
        }
      }
    }
  })";
  AnalyzerBase::ResultCode result = od_->ValidateParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kOk, result);
  result = ic_->ValidateParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kOk, result);
}

TEST_F(AnalyzerTest, ValidateParamEmpty) {
  AnalyzerBase::ResultCode result = od_->ValidateParam("");
  EXPECT_EQ(AnalyzerBase::ResultCode::kInvalidParam, result);
  result = ic_->ValidateParam("");
  EXPECT_EQ(AnalyzerBase::ResultCode::kInvalidParam, result);
}

TEST_F(AnalyzerTest, ValidateParamNoModel) {
  std::string text = R"({
  })";
  AnalyzerBase::ResultCode result = od_->ValidateParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kInvalidParam, result);
  result = ic_->ValidateParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kInvalidParam, result);
}

TEST_F(AnalyzerTest, ValidateParamNoBird) {
  std::string text = R"({
    "ai_models": {
    }
  })";
  AnalyzerBase::ResultCode result = od_->ValidateParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kInvalidParam, result);
  result = ic_->ValidateParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kInvalidParam, result);
}

TEST_F(AnalyzerTest, ValidateParamNoBundleId) {
  std::string text = R"({
    "ai_models": {
      "detection_bird": {
      },
      "classification_bird": {
      }
    }
  })";
  AnalyzerBase::ResultCode result = od_->ValidateParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kInvalidParam, result);
  result = ic_->ValidateParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kInvalidParam, result);
}

TEST_F(AnalyzerTest, ValidateParamNumericBundleId) {
  std::string text = R"({
    "ai_models": {
      "detection_bird": {
        "ai_model_bundle_id": 1
      },
      "classification_bird": {
        "ai_model_bundle_id": 1
      }
    }
  })";
  AnalyzerBase::ResultCode result = od_->ValidateParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kInvalidParam, result);
  result = ic_->ValidateParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kInvalidParam, result);
}

TEST_F(AnalyzerTest, ValidateParamShortBundleId) {
  std::string text = R"({
    "ai_models": {
      "detection_bird": {
        "ai_model_bundle_id": "1"
      },
      "classification_bird": {
        "ai_model_bundle_id": "1"
      }
    }
  })";
  AnalyzerBase::ResultCode result = od_->ValidateParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kInvalidParam, result);
  result = ic_->ValidateParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kInvalidParam, result);
}

TEST_F(AnalyzerTest, ValidateParamAlphabetBundleId) {
  std::string text = R"({
    "ai_models": {
      "detection_bird": {
        "ai_model_bundle_id": "XXXXXX"
      },
      "classification_bird": {
        "ai_model_bundle_id": "XXXXXX"
      }
    }
  })";
  AnalyzerBase::ResultCode result = od_->ValidateParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kInvalidParam, result);
  result = ic_->ValidateParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kInvalidParam, result);
}

TEST_F(AnalyzerTest, ValidateParamNoParam) {
  std::string text = R"({
    "ai_models": {
      "detection_bird": {
        "ai_model_bundle_id": "000001"
      },
      "classification_bird": {
        "ai_model_bundle_id": "000002"
      }
    }
  })";
  AnalyzerBase::ResultCode result = od_->ValidateParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kInvalidParam, result);
  result = ic_->ValidateParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kInvalidParam, result);
}

TEST_F(AnalyzerTest, ValidateParamNoDnn) {
  std::string text = R"({
    "ai_models": {
      "detection_bird": {
        "ai_model_bundle_id": "000001",
        "param": {
        }
      },
      "classification_bird": {
        "ai_model_bundle_id": "000002",
        "param": {
        }
      }
    }
  })";
  AnalyzerBase::ResultCode result = od_->ValidateParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kInvalidParam, result);
  result = ic_->ValidateParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kInvalidParam, result);
}

TEST_F(AnalyzerTest, ValidateParamNoMax) {
  std::string text = R"({
    "ai_models": {
      "detection_bird": {
        "ai_model_bundle_id": "000001",
        "param": {
        }
      },
      "classification_bird": {
        "ai_model_bundle_id": "000002",
        "param": {
        }
      }
    }
  })";
  AnalyzerBase::ResultCode result = od_->ValidateParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kInvalidParam, result);
  result = ic_->ValidateParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kInvalidParam, result);
}

TEST_F(AnalyzerTest, ValidateParamMaxOverDnn) {
  std::string text = R"({
    "ai_models": {
      "detection_bird": {
        "ai_model_bundle_id": "000001",
        "param": {
          "max_detections": 20,
          "threshold": 0.3,
          "input_width": 300,
          "input_height": 300,
          "force_switch": 1
        }
      },
      "classification_bird": {
        "ai_model_bundle_id": "000002",
        "param": {
          "max_predictions": 10
        }
      }
    }
  })";
  // only output warning
  AnalyzerBase::ResultCode result = od_->ValidateParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kOk, result);
  result = ic_->ValidateParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kOk, result);
}

TEST_F(AnalyzerTest, ValidateParam_Threshold_BoundaryCheck) {
  /*
  ** threshold < 0.0
  */
  std::string text = R"({
    "ai_models": {
      "detection_bird": {
        "ai_model_bundle_id": "000001",
        "param": {
          "max_detections": 3,
          "threshold": -0.3,
          "input_width": 300,
          "input_height": 300
        }
      },
      "classification_bird": {
        "ai_model_bundle_id": "000002",
        "param": {
          "max_predictions": 3
        }
      }
    }
  })";
  AnalyzerBase::ResultCode result = od_->ValidateParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kInvalidParam, result);

  /*
  ** 0.0 <= threshold <= 1.0
  */
  text = R"({
    "ai_models": {
      "detection_bird": {
        "ai_model_bundle_id": "000001",
        "param": {
          "max_detections": 3,
          "threshold": 0.3,
          "input_width": 300,
          "input_height": 300
        }
      },
      "classification_bird": {
        "ai_model_bundle_id": "000002",
        "param": {
          "max_predictions": 3
        }
      }
    }
  })";
  result = od_->ValidateParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kOk, result);

  /*
  ** 1.0 < threshold
  */
  text = R"({
    "ai_models": {
      "detection_bird": {
        "ai_model_bundle_id": "000001",
        "param": {
          "max_detections": 3,
          "threshold": 3.0,
          "input_width": 300,
          "input_height": 300
        }
      },
      "classification_bird": {
        "ai_model_bundle_id": "000002",
        "param": {
          "max_predictions": 3
        }
      }
    }
  })";
  result = od_->ValidateParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kInvalidParam, result);
}

TEST_F(AnalyzerTest, ValidateParamNoThresholdOnOd) {
  std::string text = R"({
    "ai_models": {
      "detection_bird": {
        "ai_model_bundle_id": "000001",
        "param": {
          "max_detections": 3
        }
      }
    }
  })";
  AnalyzerBase::ResultCode result = od_->ValidateParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kInvalidParam, result);
}

TEST_F(AnalyzerTest, ValidateParamNoWidthOnOd) {
  std::string text = R"({
    "ai_models": {
      "detection_bird": {
        "ai_model_bundle_id": "000001",
        "param": {
          "max_detections": 3,
          "threshold": 0.3
        }
      }
    }
  })";
  AnalyzerBase::ResultCode result = od_->ValidateParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kInvalidParam, result);
}

TEST_F(AnalyzerTest, ValidateParamNoHeightOnOd) {
  std::string text = R"({
    "ai_models": {
      "detection_bird": {
        "ai_model_bundle_id": "000001",
        "param": {
          "max_detections": 3,
          "threshold": 0.3,
          "input_width": 300
        }
      }
    }
  })";
  AnalyzerBase::ResultCode result = od_->ValidateParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kInvalidParam, result);
}

TEST_F(AnalyzerTest, SetValidatedParamSuccess) {
  std::string text = R"({
    "ai_models": {
      "detection_bird": {
        "ai_model_bundle_id": "000001",
        "param": {
          "max_detections": 3,
          "threshold": 0.3,
          "input_width": 300,
          "input_height": 300
        }
      },
      "classification_bird": {
        "ai_model_bundle_id": "000002",
        "param": {
          "max_predictions": 3
        }
      }
    }
  })";
  AnalyzerBase::ResultCode result = od_->ValidateParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kOk, result);
  result = od_->SetValidatedParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kOk, result);
  result = ic_->ValidateParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kOk, result);
  result = ic_->SetValidatedParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kOk, result);
}

TEST_F(AnalyzerTest, SetValidatedParamWithoutValidation) {
  std::string text = R"({
    "ai_models": {
      "detection_bird": {
        "ai_model_bundle_id": "000001",
        "param": {
          "max_detections": 3,
          "threshold": 0.3,
          "input_width": 300,
          "input_height": 300
        }
      },
      "classification_bird": {
        "ai_model_bundle_id": "000002",
        "param": {
          "max_predictions": 3
        }
      }
    }
  })";
  /* Clear parameters that is set by SetUp() */
  AnalyzerBase::ResultCode result = od_->ClearValidatingParam();
  EXPECT_EQ(AnalyzerBase::ResultCode::kOk, result);
  result = ic_->ClearValidatingParam();
  EXPECT_EQ(AnalyzerBase::ResultCode::kOk, result);
  /* Set without validation */
  result = od_->SetValidatedParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kOtherError, result);
  result = ic_->SetValidatedParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kOtherError, result);
}

TEST_F(AnalyzerTest, SetValidatedParamInvalidThreshold) {
  std::string text = R"({
    "ai_models": {
      "detection_bird": {
        "ai_model_bundle_id": "000001",
        "param": {
          "max_detections": 3,
          "threshold": -1,
          "input_width": 300,
          "input_height": 300
        }
      },
      "classification_bird": {
        "ai_model_bundle_id": "000002",
        "param": {
          "max_predictions": 3
        }
      }
    }
  })";
  AnalyzerBase::ResultCode result = od_->ValidateParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kInvalidParam, result);
}

TEST_F(AnalyzerTest, SetValidatedParamInvalidMaxDetections) {
  std::string text = R"({
    "ai_models": {
      "detection_bird": {
        "ai_model_bundle_id": "000001",
        "param": {
          "max_detections": -1,
          "threshold": 0.3,
          "input_width": 300,
          "input_height": 300
        }
      },
      "classification_bird": {
        "ai_model_bundle_id": "000002",
        "param": {
          "max_predictions": 3
        }
      }
    }
  })";
  AnalyzerBase::ResultCode result = od_->ValidateParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kInvalidParam, result);
}

TEST_F(AnalyzerTest, SetValidatedParamInvalidInputWidth) {
  std::string text = R"({
    "ai_models": {
      "detection_bird": {
        "ai_model_bundle_id": "000001",
        "param": {
          "max_detections": 3,
          "threshold": 0.3,
          "input_width": -1,
          "input_height": 300
        }
      },
      "classification_bird": {
        "ai_model_bundle_id": "000002",
        "param": {
          "max_predictions": 3
        }
      }
    }
  })";
  AnalyzerBase::ResultCode result = od_->ValidateParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kInvalidParam, result);
}

TEST_F(AnalyzerTest, SetValidatedParamInvalidInputHeight) {
  std::string text = R"({
    "ai_models": {
      "detection_bird": {
        "ai_model_bundle_id": "000001",
        "param": {
          "max_detections": 3,
          "threshold": 0.3,
          "input_width": 300,
          "input_height": -1
        }
      },
      "classification_bird": {
        "ai_model_bundle_id": "000002",
        "param": {
          "max_predictions": 3
        }
      }
    }
  })";
  AnalyzerBase::ResultCode result = od_->ValidateParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kInvalidParam, result);
}

TEST_F(AnalyzerTest, SetValidatedParamInvalidMaxPredictions) {
  std::string text = R"({
    "ai_models": {
      "detection_bird": {
        "ai_model_bundle_id": "000001",
        "param": {
          "max_detections": 3,
          "threshold": 0.3,
          "input_width": 300,
          "input_height": 300
        }
      },
      "classification_bird": {
        "ai_model_bundle_id": "000002",
        "param": {
          "max_predictions": -1
        }
      }
    }
  })";
  AnalyzerBase::ResultCode result = ic_->ValidateParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kInvalidParamOutOfRange, result);
}

TEST_F(AnalyzerTest, ClearValidatingParam) {
  AnalyzerBase::ResultCode result = od_->ClearValidatingParam();
  EXPECT_EQ(AnalyzerBase::ResultCode::kOk, result);
  result = ic_->ClearValidatingParam();
  EXPECT_EQ(AnalyzerBase::ResultCode::kOk, result);
}

TEST_F(AnalyzerTest, AnalyzeSuccess) {
  uint64_t timestamp = 12345;
  AnalyzerBase::ResultCode result =
      od_->Analyze(kOutputTensorOd, sizeof(kOutputTensorOd) / 4, timestamp);
  EXPECT_EQ(AnalyzerBase::ResultCode::kOk, result);
  result =
      ic_->Analyze(kOutputTensorIc, sizeof(kOutputTensorIc) / 4, timestamp);
  EXPECT_EQ(AnalyzerBase::ResultCode::kOk, result);
}

TEST_F(AnalyzerTest, AnalyzeNoData) {
  AnalyzerBase::ResultCode result = od_->Analyze(nullptr, 0, 0);
  EXPECT_EQ(AnalyzerBase::ResultCode::kInvalidParam, result);
  result = ic_->Analyze(nullptr, 0, 0);
  EXPECT_EQ(AnalyzerBase::ResultCode::kInvalidParam, result);
}

TEST_F(AnalyzerTest, AnalyzeEmptyData) {
  std::string text = R"({
    "ai_models": {
      "detection_bird": {
        "ai_model_bundle_id": "000001",
        "param": {
          "max_detections": 3,
          "threshold": 0.3,
          "input_width": 300,
          "input_height": 300
        }
      },
      "classification_bird": {
        "ai_model_bundle_id": "000002",
        "param": {
          "max_predictions": 3
        }
      }
    }
  })";
  ic_->ClearValidatingParam();
  ic_->ValidateParam(text.c_str());
  ic_->SetValidatedParam(text.c_str());
  uint64_t timestamp = 12345;
  const float score[] = {};
  /* size is 0 (meaningless analyze) */
  AnalyzerBase::ResultCode result = ic_->Analyze(score, 0, timestamp);
  EXPECT_EQ(AnalyzerBase::ResultCode::kOk, result);
}

TEST_F(AnalyzerTest, AnalyzeInvalidDetectionsOnOd) {
  size_t size = sizeof(kOutputTensorOd) / sizeof(kOutputTensorOd[0]);
  float tensor[size];
  for (int i = 0; i < size; ++i) {
    tensor[i] = kOutputTensorOd[i];
  }
  tensor[size - 1] = 100;
  uint64_t timestamp = 12345;
  AnalyzerBase::ResultCode result =
      od_->Analyze(tensor, sizeof(tensor) / 4, timestamp);
  // only output warning
  EXPECT_EQ(AnalyzerBase::ResultCode::kOk, result);
}

TEST_F(AnalyzerTest, AnalyzeHighThresholdOnOd) {
  std::string text = R"({
    "ai_models": {
      "detection_bird": {
        "ai_model_bundle_id": "000001",
        "param": {
          "max_detections": 3,
          "threshold": 0.9,
          "input_width": 300,
          "input_height": 300
        }
      }
    }
  })";
  AnalyzerBase::ResultCode result = od_->ValidateParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kOk, result);
  result = od_->SetValidatedParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kOk, result);

  uint64_t timestamp = 12345;
  result =
      od_->Analyze(kOutputTensorOd, sizeof(kOutputTensorOd) / 4, timestamp);

  EXPECT_EQ(AnalyzerBase::ResultCode::kOk, result);
}

TEST_F(AnalyzerTest, AnalyzeSmallMaxDetectionOnOd) {
  std::string text = R"({
    "ai_models": {
      "detection_bird": {
        "ai_model_bundle_id": "000001",
        "param": {
          "max_detections": 1,
          "threshold": 0.3,
          "input_width": 300,
          "input_height": 300
        }
      }
    }
  })";
  AnalyzerBase::ResultCode result = od_->ValidateParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kOk, result);
  result = od_->SetValidatedParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kOk, result);

  uint64_t timestamp = 12345;
  result =
      od_->Analyze(kOutputTensorOd, sizeof(kOutputTensorOd) / 4, timestamp);

  EXPECT_EQ(AnalyzerBase::ResultCode::kOk, result);
}

TEST_F(AnalyzerTest, SerializeSuccess) {
  uint64_t timestamp = 12345;
  /* Od */
  AnalyzerBase::ResultCode result =
      od_->Analyze(kOutputTensorOd, sizeof(kOutputTensorOd) / 4, timestamp);
  EXPECT_EQ(AnalyzerBase::ResultCode::kOk, result);
  void *buff = nullptr;
  uint32_t size = 0;
  result = od_->Serialize(&buff, &size, &allocator_);
  EXPECT_NE(nullptr, buff);
  allocator_.Free(buff);
  EXPECT_EQ(AnalyzerBase::ResultCode::kOk, result);
  /* Ic */
  result =
      ic_->Analyze(kOutputTensorIc, sizeof(kOutputTensorIc) / 4, timestamp);
  EXPECT_EQ(AnalyzerBase::ResultCode::kOk, result);
  buff = nullptr;
  size = 0;
  result = ic_->Serialize(&buff, &size, &allocator_);
  EXPECT_NE(nullptr, buff);
  allocator_.Free(buff);
  EXPECT_EQ(AnalyzerBase::ResultCode::kOk, result);
}

TEST_F(AnalyzerTest, SerializeInvalidFlatBuffers) {
  uint64_t timestamp = 12345;
  g_flatbuffer_normal = false;
  /* Od */
  AnalyzerBase::ResultCode result =
      od_->Analyze(kOutputTensorOd, sizeof(kOutputTensorOd) / 4, timestamp);
  EXPECT_EQ(AnalyzerBase::ResultCode::kOk, result);
  void *buff = nullptr;
  uint32_t size = 0;
  result = od_->Serialize(&buff, &size, &allocator_);
  EXPECT_EQ(nullptr, buff);
  EXPECT_EQ(AnalyzerBase::ResultCode::kOtherError, result);
  /* Ic */
  result =
      ic_->Analyze(kOutputTensorIc, sizeof(kOutputTensorIc) / 4, timestamp);
  EXPECT_EQ(AnalyzerBase::ResultCode::kOk, result);
  buff = nullptr;
  size = 0;
  result = ic_->Serialize(&buff, &size, &allocator_);
  EXPECT_EQ(nullptr, buff);
  EXPECT_EQ(AnalyzerBase::ResultCode::kOtherError, result);
}

TEST_F(AnalyzerTest, SerializeNoMalloc) {
  void *buff = nullptr;
  uint32_t size = 0;
  AnalyzerBase::ResultCode result =
      od_->Serialize(&buff, &size, &invalid_allocator_);
  EXPECT_EQ(nullptr, buff);
  invalid_allocator_.Free(buff);  // not necessary but ok
  EXPECT_EQ(AnalyzerBase::ResultCode::kMemoryError, result);
  result = ic_->Serialize(&buff, &size, &invalid_allocator_);
  EXPECT_EQ(nullptr, buff);
  invalid_allocator_.Free(buff);  // not necessary but ok
  EXPECT_EQ(AnalyzerBase::ResultCode::kMemoryError, result);
}

TEST_F(AnalyzerTest, GetNetworkId) {
  char network_id[AI_MODEL_BUNDLE_ID_SIZE] = {"0"};
  char expected_network_id[AI_MODEL_BUNDLE_ID_SIZE] = {"000001"};
  char expected_network_id2[AI_MODEL_BUNDLE_ID_SIZE] = {"000002"};
  AnalyzerBase::ResultCode result = od_->GetNetworkId(network_id);
  EXPECT_EQ(
      0, strncmp(network_id, expected_network_id, strlen(expected_network_id)));
  EXPECT_EQ(AnalyzerBase::ResultCode::kOk, result);
  result = ic_->GetNetworkId(network_id);
  EXPECT_EQ(0, strncmp(network_id, expected_network_id2,
                       strlen(expected_network_id2)));
  EXPECT_EQ(AnalyzerBase::ResultCode::kOk, result);
}

TEST_F(AnalyzerTest, GetInputTensorSize) {
  uint16_t width = 0;
  uint16_t height = 0;
  AnalyzerBase::ResultCode result = od_->GetInputTensorSize(width, height);
  EXPECT_EQ(300, width);
  EXPECT_EQ(300, height);
  EXPECT_EQ(AnalyzerBase::ResultCode::kOk, result);
}

TEST_F(AnalyzerTest, SetNetworkIdTooLong) {
  std::string text = R"({
    "ai_models": {
      "detection_bird": {
        "ai_model_bundle_id": "12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678",
        "param": {
          "max_detections": 3,
          "threshold": 0.3,
          "input_width": 300,
          "input_height": 300
        }
      },
      "classification_bird": {
        "ai_model_bundle_id": "000002",
        "param": {
          "max_predictions": 3
        }
      }
    }
  })";

  AnalyzerBase::ResultCode result = od_->ValidateParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kInvalidParam, result);
  char network_id[AI_MODEL_BUNDLE_ID_SIZE] = {"0"};
  result = od_->GetNetworkId(network_id);
  EXPECT_EQ(0, strncmp(network_id, "", strlen("")));

  text = R"({
    "ai_models": {
      "detection_bird": {
        "ai_model_bundle_id": "000001",
        "param": {
          "max_detections": 3,
          "threshold": 0.3,
          "input_width": 300,
          "input_height": 300
        }
      },
      "classification_bird": {
        "ai_model_bundle_id": "12345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789012345678",
        "param": {
          "max_predictions": 3
        }
      }
    }
  })";

  result = ic_->ValidateParam(text.c_str());
  EXPECT_EQ(AnalyzerBase::ResultCode::kInvalidParam, result);
  char network_id_ic[AI_MODEL_BUNDLE_ID_SIZE] = {"0"};
  result = ic_->GetNetworkId(network_id_ic);
  EXPECT_EQ(0, strncmp(network_id_ic, "", strlen("")));
}
