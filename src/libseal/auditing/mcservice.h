/*
 * Copyright 2018 Imperial College London
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at   
 * 
 *      http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MCSERVICE_H_
#define MCSERVICE_H_

#include "enclaveshim_config.h"

// they need to always be defined because
// they are listed in enclave.edl
#ifdef COMPILE_WITH_INTEL_SGX
void ecall_mcservice_initialize(int q);
#else
void ocall_mcservice_network_round(void);
#endif

#ifdef USE_MONOTONIC_COUNTER_SERVICE
#ifdef COMPILE_WITH_INTEL_SGX
void mcservice_encrypt_round(void);
void mcservice_decrypt_round(void);
void mcservice_network_round(void); // this one makes an ocall
#else
int mcservice_initialize(void);
void mcservice_network_round(void);
#endif

#endif

#endif
