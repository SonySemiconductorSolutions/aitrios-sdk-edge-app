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

#if !defined(__SDK_BLOB_HTTP_EXT_H__)
#define __SDK_BLOB_HTTP_EXT_H__

#if defined(__cplusplus)
extern "C" {
#endif

/** @file */

/**
 * @brief A blob operation request for ordinary HTTP server, supporting extra
 * headers.
 */
struct EVP_BlobRequestHttpExt;

/**
 * @brief A blob operation result for HTTP server.
 */
struct EVP_BlobResultHttpExt {
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

/** @brief Initializes an EVP_BlobRequestHttpExt
 *
 * This function must be called when instantiating an EVP_BlobRequestHttpExt.
 * It returns a pointer to a new request that must be later freed using
 * EVP_BlobRequestHttpExt_free
 *
 * @return @ref Pointer to a newly allocated request struct. NULL on failure.
 */
struct EVP_BlobRequestHttpExt *EVP_BlobRequestHttpExt_initialize(void);

/** @brief Frees an EVP_BlobRequestHttpExt
 *
 * This function must be called when freeing an EVP_BlobRequestHttpExt
 *
 * @param request     A pointer to a EVP_BlobRequestHttpExt structure.
 */
void EVP_BlobRequestHttpExt_free(struct EVP_BlobRequestHttpExt *request);
/** @brief Inserts an extra header to EVP_BlobRequestHttpExt
 *
 * This helper function inserts an extra header into the request.
 *
 * @param request   A pointer to a EVP_BlobRequestHttpExt structure.
 * @param name     	A pointer to a null-terminated string containing the
 * name of the header.
 * @param value     A pointer to a null-terminated string containing the value
 * of the header.
 *
 * @return @ref EVP_OK Success.
 */
EVP_RESULT
EVP_BlobRequestHttpExt_addHeader(struct EVP_BlobRequestHttpExt *request,
                                 const char *name, const char *value);

/** @brief Inserts an extra header to EVP_BlobRequestHttpExt
 *
 * This helper function inserts the azure specific headers in the request.
 *
 * @param request   A pointer to a EVP_BlobRequestHttpExt structure.
 *
 * @return @ref EVP_OK Success.
 */
EVP_RESULT
EVP_BlobRequestHttpExt_addAzureHeader(struct EVP_BlobRequestHttpExt *request);

/** @brief Sets the url of EVP_BlobRequestHttpExt
 *
 * This function sets the url of the request.
 *
 * @param request   A pointer to a EVP_BlobRequestHttpExt structure.
 * @param url		The destination URL of the request.
 *
 * @return @ref EVP_OK Success.
 */
EVP_RESULT
EVP_BlobRequestHttpExt_setUrl(struct EVP_BlobRequestHttpExt *request,
                              char *url);

#if defined(__cplusplus)
} /* extern "C" */
#endif

#endif /* !defined(__SDK_BLOB_HTTP_EXT_H__) */
