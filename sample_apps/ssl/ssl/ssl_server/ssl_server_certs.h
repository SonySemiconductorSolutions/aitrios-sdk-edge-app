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

#ifndef SSL_SERVER_CERTS_H
#define SSL_SERVER_CERTS_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief SSL Server Certificate Management
 *
 * This header provides access to SSL server test certificates and keys
 * for development and testing purposes.
 *
 * Note: These are test certificates and should not be used
 * in production environments.
 */

// Certificate and key declarations
extern const char mbedtls_test_srv_crt[];
extern const char mbedtls_test_srv_key[];
extern const char mbedtls_test_cas_pem[];

extern const size_t mbedtls_test_srv_crt_len;
extern const size_t mbedtls_test_srv_key_len;
extern const size_t mbedtls_test_cas_pem_len;

#ifdef __cplusplus
}
#endif

#endif /* SSL_SERVER_CERTS_H */
