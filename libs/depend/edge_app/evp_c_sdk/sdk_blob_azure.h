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

#if !defined(__SDK_BLOB_AZURE_H__)
#define __SDK_BLOB_AZURE_H__

#if defined(__cplusplus)
extern "C" {
#endif

/** @file */

/**
 * @brief A blob operation request for Azure Blob Storage.
 */
struct EVP_BlobRequestAzureBlob {
  /**
   * Shared Access Signature URL for the blob.
   *
   * @ref EVP_BLOB_OP_GET requires `Read (r)` permission.
   *
   * @ref EVP_BLOB_OP_PUT requires `Create (c)` and/or `Write (w)`
   * permission.
   *
   * @see
   * https://docs.microsoft.com/en-us/rest/api/storageservices/create-service-sas
   */
  const char *url;
};

/**
 * @brief A blob operation result for Azure Blob Storage.
 */
struct EVP_BlobResultAzureBlob {
  /**
   * The result of the blob operation.
   */
  EVP_BLOB_RESULT result;

  /**
   * An HTTP status code.
   */
  unsigned int http_status;

  /**
   * An errno value.
   * Only valid for @ref EVP_BLOB_RESULT_ERROR.
   */
  int error;
};

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* !defined(__SDK_BLOB_AZURE_H__) */
