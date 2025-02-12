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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "base64.h"

int test_base64_encode(size_t input_length, uint8_t *data,
                       uint8_t *expected_output) {
  char *b64_input = malloc(input_length + 1);
  if (!b64_input) {
    printf("Failed to allocate memory for the test.\n");
    return -1;
  }

  strcpy(b64_input, (char *)data);

  char *b64_output = malloc(((input_length + 2) / 3) * 4 + 1);
  if (!b64_output) {
    printf("Failed to allocate memory for the test.\n");
    free(b64_input);
    return -1;
  }

  printf("Starting base64 encoding in native code...\n");

  uint32_t encoded_data = b64_encode(b64_input, input_length, b64_output);
  if (encoded_data == 0) {
    printf("Failed to encode the data.\n");
    free(b64_input);
    free(b64_output);
    return -1;
  }

  printf("Encoded value (Native) is %s\n", b64_output);
  if (strcmp(b64_output, (char *)expected_output) != 0) {
    fprintf(stderr, "Test failed. Expected %s but got %s\n", expected_output,
            b64_output);
  } else {
    printf("Test passed\n");
  }

  free(b64_input);
  free(b64_output);

  return 0;
}

int main() {
  unsigned char *data1 = "Hello, World!";
  unsigned char *expected_output1 = "SGVsbG8sIFdvcmxkIQ==";

  unsigned char *data2 =
      "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod "
      "tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim "
      "veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea "
      "commodo consequat. Duis aute irure dolor in reprehenderit in voluptate "
      "velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint "
      "occaecat cupidatat non proident, sunt in culpa qui officia deserunt "
      "mollit anim id est laborum.";
  unsigned char *expected_output2 =
      "TG9yZW0gaXBzdW0gZG9sb3Igc2l0IGFtZXQsIGNvbnNlY3RldHVyIGFkaXBpc2NpbmcgZWxp"
      "dCwgc2VkIGRvIGVpdXNtb2QgdGVtcG9yIGluY2lkaWR1bnQgdXQgbGFib3JlIGV0IGRvbG9y"
      "ZSBtYWduYSBhbGlxdWEuIFV0IGVuaW0gYWQgbWluaW0gdmVuaWFtLCBxdWlzIG5vc3RydWQg"
      "ZXhlcmNpdGF0aW9uIHVsbGFtY28gbGFib3JpcyBuaXNpIHV0IGFsaXF1aXAgZXggZWEgY29t"
      "bW9kbyBjb25zZXF1YXQuIER1aXMgYXV0ZSBpcnVyZSBkb2xvciBpbiByZXByZWhlbmRlcml0"
      "IGluIHZvbHVwdGF0ZSB2ZWxpdCBlc3NlIGNpbGx1bSBkb2xvcmUgZXUgZnVnaWF0IG51bGxh"
      "IHBhcmlhdHVyLiBFeGNlcHRldXIgc2ludCBvY2NhZWNhdCBjdXBpZGF0YXQgbm9uIHByb2lk"
      "ZW50LCBzdW50IGluIGN1bHBhIHF1aSBvZmZpY2lhIGRlc2VydW50IG1vbGxpdCBhbmltIGlk"
      "IGVzdCBsYWJvcnVtLg==";

  size_t input_length1 = strlen(data1);
  size_t input_length2 = strlen(data2);

  int ret = test_base64_encode(input_length1, data1, expected_output1);
  if (ret != 0) {
    return ret;
  }

  ret = test_base64_encode(input_length2, data2, expected_output2);
  return ret;
}
