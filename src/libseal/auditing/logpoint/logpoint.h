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

#ifndef LOGPOINT_H
#define LOGPOINT_H

#include <stdio.h>
#include <stdlib.h>

#ifdef COMPILE_WITH_INTEL_SGX
	#include "sgx_error.h"
	extern int my_fprintf(FILE *stream, const char *format, ...);
	extern sgx_status_t ocall_exit(int s);
#else
	#define my_fprintf(fd, format, ...) printf(format, ##__VA_ARGS__)
#endif

void logpoint_log(char *req, char *rsp, unsigned int req_len, unsigned int rsp_len);
void logpoint_init(void);

#endif

