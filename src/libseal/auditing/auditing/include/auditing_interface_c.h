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

#ifndef AUDITING_INTERFACE_C_H_
#define AUDITING_INTERFACE_C_H_

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*insert_statement_fn)(int table, const char* tuple);
typedef void (*execute_stmt_fn)(const char* invariant);

/*
 * Returns a pointer that must be freed by the caller.
 */
void libseal_process_log_at_runtime(char *req, char *rsp, unsigned int req_len, unsigned int rsp_len, insert_statement_fn insert_stmt_fn);

/*
 * Returns a pointer that must be freed by the caller.
 */
char *libseal_init_relations();

void libseal_do_audit(execute_stmt_fn execute_stmt_fn);

void libseal_do_trimming(execute_stmt_fn execute_stmt_fn);

const char **get_tables();

#ifdef __cplusplus
}
#endif

#endif /* AUDITING_INTERFACE_H_ */
