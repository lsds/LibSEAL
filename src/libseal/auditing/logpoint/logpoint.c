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

#include <sha.h>
#include <string.h>

#include "enclaveshim_config.h"

#ifdef USE_MONOTONIC_COUNTER_SERVICE 
#include "mcservice.h"
#endif

#ifdef COMPILE_WITH_INTEL_SGX
#ifdef SQLITE_DO_SIGN
#include "sgx_tcrypto.h"
#endif /* SQLITE_DO_SIGN */
#else
	#include <stdio.h>
	#include <stdlib.h>
	#include <dlfcn.h>
	#define ocall_exit(s) exit(s)
#endif /* COMPILE_WITH_INTEL_SGX */

#ifdef COMPILE_WITH_INTEL_SGX
	#include <sgx_thread.h>
	#define THREAD_MUTEX_INITIALIZER SGX_THREAD_MUTEX_INITIALIZER
	#define pthread_mutex_lock(m) sgx_thread_mutex_lock(m)
	#define pthread_mutex_unlock(m) sgx_thread_mutex_unlock(m)
	typedef sgx_thread_mutex_t thread_mutex_t;
#else
	#include <pthread.h>
	#define THREAD_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
	#define my_fprintf(fd, format, ...) printf(format, ##__VA_ARGS__)
	typedef pthread_mutex_t thread_mutex_t;
#endif

#include "sqlite3.h"
#include "logpoint.h"
#include "auditing_interface_c.h"


static sqlite3 *db = NULL;
static int initialised = 0;
char lasthash[65] = { 0 };
const char **thetables;

#ifdef SQLITE_DO_INSERT
static thread_mutex_t lasthash_mutex = THREAD_MUTEX_INITIALIZER;
static thread_mutex_t sqlite_mutex = THREAD_MUTEX_INITIALIZER;
#endif

#ifdef SQLITE_DO_CHECK
int docheck = 0;
#endif

#if defined(COMPILE_WITH_INTEL_SGX) && defined(SQLITE_DO_SIGN) && defined(SQLITE_ADD_HASH)
static sgx_ec256_private_t p_private = {{0}};
static sgx_ec256_public_t p_public = {{0}, {0}};
#ifdef SQLITE_DO_INSERT
static sgx_ec256_signature_t p_signature;
#endif
static sgx_ecc_state_handle_t ecc_handle;
#endif

#ifndef COMPILE_WITH_INTEL_SGX
static void* libhandler;
#endif

char* (*libseal_init_relations_f)(void) = NULL;
void (*libseal_process_log_at_runtime_f)(char*, char*, unsigned int, unsigned int, insert_statement_fn) = NULL;
void (*libseal_do_audit_f)(execute_stmt_fn execute_stmt_fn) = NULL;
void (*libseal_do_trimming_f)(execute_stmt_fn execute_stmt_fn) = NULL;
const char** (*get_tables_f)(void) = NULL;

/*
static int callback(void *NotUsed, int argc, char **argv, char **azColName){
	int i;
	for(i=0; i<argc; i++){
		my_fprintf(0, "%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	}
	my_fprintf(0,"\n");
	return 0;
}
*/

void sha256(char *string, char outputBuffer[65]) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, string, strlen(string));
    SHA256_Final(hash, &sha256);
    int i = 0;
    for(i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        snprintf(outputBuffer + (i * 2), 65, "%02x", hash[i]);
    }
    outputBuffer[64] = 0;
}

void execute_stmt_tx(const char *stmt) {
#ifdef SQLITE_DO_INSERT
	char *zErrMsg = 0;

	pthread_mutex_lock(&sqlite_mutex);
	//my_fprintf(0, "Executing TX: %s\n", stmt);
	if (sqlite3_exec(db, "begin transaction;", 0, 0, &zErrMsg) != SQLITE_OK){
		my_fprintf(stderr, "SQL error: %s, statement: %s\n", zErrMsg, stmt);
		sqlite3_free(zErrMsg);
	}
	if (sqlite3_exec(db, stmt, 0, 0, &zErrMsg) != SQLITE_OK){
		my_fprintf(stderr, "SQL error: %s, statement: %s\n", zErrMsg, stmt);
		sqlite3_free(zErrMsg);
	}
	if (sqlite3_exec(db, "end transaction;", 0, 0, &zErrMsg) != SQLITE_OK){
		my_fprintf(stderr, "SQL error: %s, statement: %s\n", zErrMsg, stmt);
		sqlite3_free(zErrMsg);
	}
	pthread_mutex_unlock(&sqlite_mutex);
#endif
}

void execute_stmt(const char *stmt) {
#ifdef SQLITE_DO_INSERT
	char *zErrMsg = 0;

	pthread_mutex_lock(&sqlite_mutex);
	//my_fprintf(0, "Executing: %s\n", stmt);
	if (sqlite3_exec(db, stmt, 0, 0, &zErrMsg) != SQLITE_OK){
		my_fprintf(stderr, "SQL error: %s, statement: %s\n", zErrMsg, stmt);
		sqlite3_free(zErrMsg);
	}
	pthread_mutex_unlock(&sqlite_mutex);
#endif
}

#ifdef SQLITE_ADD_HASH
void add_hash_column(const char *table) {
	size_t len;

	char *before = "ALTER TABLE `";
	char *after = "` ADD `libseal_hash` TEXT;";

	len = strlen(before) + strlen(table) + strlen(after) + 1;

	char stmt[len];
	snprintf(stmt, len, "%s%s%s", before, table, after);

	execute_stmt(stmt);
}
#endif

void dl_load_symbols() {
#ifdef COMPILE_WITH_INTEL_SGX
	libseal_init_relations_f = libseal_init_relations;
	libseal_process_log_at_runtime_f = libseal_process_log_at_runtime;
	libseal_do_audit_f = libseal_do_audit;
	libseal_do_trimming_f = libseal_do_trimming;
	get_tables_f = get_tables;
#else
	/* Prepare */
	void* addr;
	char* err;
	libhandler = dlopen("auditing.so", RTLD_NOW | RTLD_LOCAL);
	if (!libhandler) {
		my_fprintf(0, "Cannot open shared library auditing.so: %s\n",
				dlerror());
		ocall_exit(1);
	}

	/* libseal_init_relations */
	dlerror(); // clear existing errors
	addr = dlsym(libhandler, "libseal_init_relations");
	err = dlerror();
	if (err) {
		my_fprintf(0, "dlsym error: %s\n", err);
		ocall_exit(1);
	}
	libseal_init_relations_f = (char* (*)(void)) addr;

	/* libseal_do_audit */
	dlerror(); // clear existing errors
	addr = dlsym(libhandler, "libseal_do_audit");
	err = dlerror();
	if (err) {
		my_fprintf(0, "dlsym error: %s\n", err);
		ocall_exit(1);
	}
	libseal_do_audit_f = (void (*)(execute_stmt_fn execute_stmt_fn)) addr;

	/* libseal_do_audit */
	dlerror(); // clear existing errors
	addr = dlsym(libhandler, "libseal_do_trimming");
	err = dlerror();
	if (err) {
		my_fprintf(0, "dlsym error: %s\n", err);
		ocall_exit(1);
	}
	libseal_do_trimming_f = (void (*)(execute_stmt_fn execute_stmt_fn)) addr;

	/* libseal_process_log_at_runtime */
	dlerror(); // clear existing errors
	addr = dlsym(libhandler, "libseal_process_log_at_runtime");
	err = dlerror();
	if (err) {
		my_fprintf(0, "dlsym error: %s\n", err);
		ocall_exit(1);
	}
	libseal_process_log_at_runtime_f = (void (*)(char*, char*, unsigned int, unsigned int, insert_statement_fn)) addr;

	/* get_tables */
	dlerror(); // clear existing errors
	addr = dlsym(libhandler, "get_tables");
	err = dlerror();
	if (err) {
		my_fprintf(0, "dlsym error: %s\n", err);
		ocall_exit(1);
	}
	get_tables_f = (const char** (*)(void)) addr;
#endif
	thetables = get_tables_f();
}

void logpoint_init(void) {
	if (initialised) {
		return;
	}
	initialised = 1;

	my_fprintf(0, "Opening Sqlite database (%s) ...\n", SQLITE_DB_NAME);
	//if(sqlite3_open_v2(SQLITE_DB_NAME, &db, SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE, NULL)) {
	if(sqlite3_open(SQLITE_DB_NAME, &db)) {
		my_fprintf(0, "%s:%i Unable to open Sqlite database (%s): %s.\n", __func__, __LINE__, SQLITE_DB_NAME, sqlite3_errmsg(db));
		ocall_exit(1);
	}

	dl_load_symbols();

#ifdef COMPILE_WITH_INTEL_SGX
	// disable shared memory in sqlite
    // this is mandetory for sgx
	execute_stmt("PRAGMA main.locking_mode = EXCLUSIVE;");

#ifdef SQLITE_ASYNC_MODE
	execute_stmt("PRAGMA journal_mode = WAL;");
    // The WAL journaling mode uses a write-ahead log instead of a rollback journal to implement transactions. 
    // The WAL journaling mode is persistent; after being set it stays in effect across multiple database connections and after closing and reopening the database. 
    
    execute_stmt("PRAGMA wal_autocheckpoint = 1000;"); // decrease this to perisist more often, default: 1000
    // Number of altered pages after which sqlite will write to the file

	execute_stmt("PRAGMA main.synchronous = NORMAL;");
    // Normal: The SQLite database engine will sync at the most critical moments, but less often than in FULL mode. 
    // There is a very small (though non-zero) chance that a power failure at just the wrong time could corrupt the database in NORMAL mode. 
    // In practice, you are more likely to suffer a catastrophic disk failure or some other unrecoverable hardware fault. 
    // Many applications choose NORMAL when in WAL mode.
#else
    // sync-mode in sgx

	//doesn't exist with our version of sqlite 
	//execute_stmt("PRAGMA schema.synchronous = OFF"); // it's fine as we increment the monotonic counter and don't have any control over what the OS is doing anyway
	// With synchronous OFF (0), SQLite continues without syncing as soon as it has handed data off to the operating system.
	// If the application running SQLite crashes, the data will be safe, but the database might become corrupted if the operating
	// system crashes or the computer loses power before that data has been written to the disk surface. On the other hand, commits
	// can be orders of magnitude faster with synchronous OFF. 

	execute_stmt("PRAGMA journal_mode = OFF"); // we don't have transactions so we don't need a journal
	//execute_stmt("PRAGMA journal_mode = MEMORY"); // in-memory rollback-journal
    // The MEMORY journaling mode stores the rollback journal in volatile RAM.
    // This saves disk I/O but at the expense of database safety and integrity. 
    // If the application using SQLite crashes in the middle of a transaction when the MEMORY journaling mode is set, 
    // then the database file will very likely go corrupt.

    //execute_stmt("PRAGMA main.journal_size_limit = 1000"); // decrease this to perisist more often, default: 1000
    // The journal_size_limit pragma may be used to limit the size of rollback-journal and WAL files left in the file-system after transactions or checkpoints. 
    // Each time a transaction is committed or a WAL file resets, 
    // SQLite compares the size of the rollback journal file or WAL file left in the file-system to the size limit set by this pragma 
    // and if the journal or WAL file is larger it is truncated to the limit

	execute_stmt("PRAGMA temp_store = MEMORY"); // store the temporary tables/views in memory

#endif

#endif


#ifdef COMPILE_WITH_INTEL_SGX
#ifdef USE_MONOTONIC_COUNTER_SERVICE
	mcservice_initialize();
#endif
#endif

	char *statements = libseal_init_relations_f();
	if (strlen(statements) > 0) {
		execute_stmt(statements);
		free(statements);
	}

#ifdef SQLITE_ADD_HASH
	int i = 0;
	while (thetables[i] != 0) {
		add_hash_column(thetables[i]);
		i++;
	}
#endif

#if defined(COMPILE_WITH_INTEL_SGX) && defined(SQLITE_DO_SIGN) && defined(SQLITE_ADD_HASH)
	sgx_status_t ret;

	ret = sgx_ecc256_open_context(&ecc_handle);
	if (ret != SGX_SUCCESS) {
		my_fprintf(0, "Error %d with sgx_ecc256_open_context\n", ret);
		return;
	}

	ret = sgx_ecc256_create_key_pair(&p_private, &p_public, ecc_handle);
	if (ret != SGX_SUCCESS) {
		my_fprintf(0, "Error %d with sgx_ecc256_create_key_pair\n", ret);
		return;
	}

	execute_stmt("CREATE TABLE `libseal_signatures` (`hash` TEXT NOT NULL, `signature_x` TEXT NOT NULL, `signature_y` TEXT NOT NULL, PRIMARY KEY(`hash`));");
#endif
}

char *create_insert_statement(const char *table, const char *tuple, size_t* stmtlen) {
	char *stmt;
	*stmtlen = strlen("INSERT INTO `") + strlen(table) + strlen("` VALUES ") + strlen(tuple) + 4;

#if defined(COMPILE_WITH_INTEL_SGX) && defined(SQLITE_DO_SIGN) && defined(SQLITE_ADD_HASH)
	//pre-allocate memory for the signature (added outside of this function)
	*stmtlen += 244;
#endif
#ifdef SQLITE_ADD_HASH
	*stmtlen += 67; // 67 for SHA256 + quotes + comma
	stmt = malloc(*stmtlen * sizeof(*stmt));

	char newhash[65];
	char hashthis[65 + strlen(table) + strlen(tuple)];

	snprintf(hashthis, 65 + strlen(table) + strlen(tuple), "%s%s%s", lasthash, table, tuple);
	sha256(hashthis, newhash);
	snprintf(stmt, *stmtlen, "INSERT INTO `%s` VALUES (%s,\"%s\");", table, tuple, newhash);
	memcpy(lasthash, newhash, 65);
#else
	stmt = malloc(*stmtlen * sizeof(*stmt));
	snprintf(stmt, *stmtlen, "INSERT INTO `%s` VALUES (%s);", table, tuple);
#endif
	return stmt;
}

void insert_statement(int table, const char* tuple) {
#ifdef SQLITE_DO_INSERT
	pthread_mutex_lock(&lasthash_mutex);

	size_t stmtlen;
	char *stmt = create_insert_statement(thetables[table], tuple, &stmtlen);

#if defined(COMPILE_WITH_INTEL_SGX) && defined(SQLITE_DO_SIGN) && defined(SQLITE_ADD_HASH)
	char* signature_insert = stmt + stmtlen - 244;
	if (lasthash[0] != 0) {
		sgx_status_t ret;
		ret = sgx_ecdsa_sign((const uint8_t*)lasthash, 64, &p_private, &p_signature, ecc_handle);
		if (ret != SGX_SUCCESS) {
			my_fprintf(0, "Error %d with sgx_ecdsa_sign\n", ret);
			return;
		}

		snprintf(signature_insert, 243, "INSERT INTO `libseal_signatures` VALUES(\"%s\",\"%x%x%x%x%x%x%x%x\",\"%x%x%x%x%x%x%x%x\");",
				lasthash,
				p_signature.x[0], p_signature.x[1], p_signature.x[2], p_signature.x[3], p_signature.x[4], p_signature.x[5], p_signature.x[6], p_signature.x[7],
				p_signature.y[0], p_signature.y[1], p_signature.y[2], p_signature.y[3], p_signature.y[4], p_signature.y[5], p_signature.y[6] ,p_signature.y[7]);
	}
#endif /* defined(COMPILE_WITH_INTEL_SGX) && defined(SQLITE_DO_SIGN) && defined(SQLITE_ADD_HASH) */

	pthread_mutex_unlock(&lasthash_mutex);

#ifdef COMPILE_WITH_INTEL_SGX
#ifdef USE_MONOTONIC_COUNTER_SERVICE
	// round 1
	mcservice_encrypt_round();
	mcservice_network_round();
	mcservice_decrypt_round();

	// round 2
	mcservice_encrypt_round();
	mcservice_network_round();
	mcservice_decrypt_round();
#endif
#endif

	execute_stmt_tx(stmt);
	free(stmt);

#endif /* SQLITE_DO_INSERT */
}

void logpoint_log(char *req, char *rsp, unsigned int req_len, unsigned int rsp_len) {
	libseal_process_log_at_runtime_f(req, rsp, req_len, rsp_len, insert_statement);

#ifdef SQLITE_DO_CHECK
	__atomic_fetch_add(&docheck, 1, __ATOMIC_RELAXED);
	int expected = SQLITE_CHECKAFTER;
	int desired = 0;
	/*
	 * if (docheck == expected) docheck = desired; return true;
	 * else expected = docheck; return false;
	 */
	if (__atomic_compare_exchange_n(&docheck, &expected, desired, 0, __ATOMIC_RELAXED, __ATOMIC_RELAXED)) {
		libseal_do_audit_f(execute_stmt_tx);
#ifdef SQLITE_DO_TRIMMING
		libseal_do_trimming_f(execute_stmt_tx);
#endif
	}
#endif
}
