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

#include <string>
#include <vector>

#include <stdio.h>
#include <stdlib.h>

#include "auditing_interface_c.h"
#include "zlib.h"
#include "jsmn.h"
#include "json_helper.h"
#include "base64.h"
#include "events.h"
#include "http.h"

#ifdef COMPILE_WITH_INTEL_SGX
extern "C" {
extern int my_printf(const char *format, ...);
}
#else
#include <stdarg.h>
void my_printf(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);
}
#endif

static unsigned long long monotonic_counter = 0;

std::string decode_url_encoding(std::string str) {
	std::string res = "";
	char v[3] = "xx";

	size_t i = 0;
	while (i < str.length()) {
		if (str[i] == '-') {
			res += '+';
			i++;
		} else if (str[i] == '_') {
			res += '/';
			i++;
		} else if (str[i] == '~') {
			res += '=';
			i++;
		} else if (str[i] == '%') {
			v[0] = str[i+1];
			v[1] = str[i+2];
			int ii = (int)strtol(v, NULL, 16);
			res += static_cast<char>(ii);
			i += 3;
		} else {
			res += str[i];
			i++;
		}
	}

	return res;
}

vector<std::string> parse_commit_batch(std::string content) {
	size_t start, stop;
	int ret;
	std::vector<std::string> statements;

	// 1. find the host_key
	start = content.find("host_key=") + 9;
	stop = content.find('&', start);
	std::string host_key = content.substr(start, stop-start);

	// 2. find the commit_info
	start = content.find("commit_info=") + 12; 
	stop = content.find('&', start);
	std::string commit_info = content.substr(start, stop-start);

	// 3. decode the commit info
	// 3.1 translate
	commit_info = decode_url_encoding(commit_info);

	// 3.2 base64 decode
	commit_info = base64_decode(commit_info);

	// 3.3 zlib decompress
	size_t max_size = 8192;
	char decompressed[max_size];
	ret = uncompress((unsigned char*)decompressed, &max_size, (unsigned char*)commit_info.c_str(), commit_info.length());
	if (ret != Z_OK) {
		my_printf("zlib error: %d (Z_MEM_ERROR=%d, Z_BUF_ERROR=%d)\n", ret, Z_MEM_ERROR, Z_BUF_ERROR);
		return statements;
	}

	commit_info = std::string(decompressed);

	//commit_info might contain more stuff, so trim it
	int nbrackets = 0;
	unsigned int i = 0;
	for (; i<commit_info.length(); i++) {
		if (commit_info[i] == '[') {
			nbrackets++;
		} else if (commit_info[i] == ']' && --nbrackets == 0) {
			break;
		}
	}
	commit_info = commit_info.substr(0, i+1);

	const char* commit_info_str = commit_info.c_str();

	// 3.4 to json
	jsmn_parser parser;
	int nbtokens = jsonGetNrTokens(&parser, commit_info_str, commit_info.length());
	if (nbtokens <= 0) {
		my_printf("commit info json parsing error: [%s]\n", commit_info_str);
		return statements;
	}

	jsmntok_t tokens[nbtokens];
	jsonReadTokens(&parser, commit_info_str, commit_info.length(), tokens, nbtokens);

	// 4. process the commit_info
	if (tokens[0].type != JSMN_ARRAY) {
		my_printf("commit info json parsing error: this is not an array: [%s]\n", commit_info_str);
		return statements;
	}

	// sometimes doing something stupid like that works
	// better than something more clever
	int done = 0;
	int size = 0;
	std::string path = "";
	std::string blocklist = "";
	for (int i=0; i<nbtokens; i++) {
		jsmntok_t *t = &tokens[i];

		std::string key = jsonGetString(t, commit_info_str);
		if (key == "blocklist") {
			blocklist = jsonGetString(t+1, commit_info_str);
			done++;
			i++;
		} else if (key == "path") {
			path = jsonGetString(t+1, commit_info_str);
			done++;
			i++;
		} else if (key == "size") {
			size = jsonGetInt(t+1, commit_info_str);
			done++;
			i++;
		}

		if (done == 3) {
			std::string statement = "\"8796754824148\", \"" + host_key + "\", \"" + path + "\", " + to_str(size) + ", \"" + blocklist + "\"";
			statements.push_back(statement);
			done = 0;
		}
	}

	return statements;
}

std::string parse_store_batch(std::string content) {
	int length;
	size_t start, stop;

	//0. find host key
	std::string host_key = "";

	// 1. read the length
	start = content.find("Content-Length:") + 16; 
	stop = content.find('\r', start);
	std::string length_as_str = content.substr(start, stop-start);
	length = (int)atoi((char*)length_as_str.c_str());
	my_printf("length=%d [%s]\n", length, length_as_str.c_str());

	// 2. read the hash
	// 2.1 find the boundary
	start = content.find("boundary=") + 9; 
	stop = content.find('\r', start);
	std::string boundary = content.substr(start, stop-start);	

	// 2.2 find the batch_info
	start = content.find("batch_info\"") + 11; 
	stop = content.find(boundary, start);
	std::string batch_info = content.substr(start, stop-start);	

	const char* batch_info_str = batch_info.c_str();

	// 2.4 to json
	jsmn_parser parser;
	int nbtokens = jsonGetNrTokens(&parser, batch_info_str, batch_info.length());
	if (nbtokens <= 0) {
		my_printf("batch info json parsing error: %s\n", batch_info_str);
		return "";
	}

	jsmntok_t tokens[nbtokens];
	jsonReadTokens(&parser, batch_info_str, batch_info.length(), tokens, nbtokens);

	// 3. process the batch_info
	if (tokens[0].type != JSMN_ARRAY && tokens[1].type != JSMN_OBJECT) {
		my_printf("batch info json parsing error: incorrect types for: [%s]\n", batch_info_str);
		return "";
	}

	std::string hash = "";
	jsmntok_t *t = &tokens[1];
	for (int k=0; k<t->size*2; k+=2) {
		jsmntok_t *u = &tokens[1+k];
		std::string key = jsonGetString(u, batch_info_str);
		if (key == "hash") {
			hash = jsonGetString(u+1, batch_info_str);
			break;
		}
	}
	my_printf("store batch hash \"%s\"\n", hash.c_str());

	return hash;
}

std::vector<std::string> parse_list(std::string req, std::string content) {
	size_t start, stop;
	std::vector<std::string> statements;

	// 1. find the host_key
	start = req.find("host_key=") + 9;
	stop = req.find('&', start);
	std::string host_key = req.substr(start, stop-start);

	// 2. find the beginning of the json map
	start = content.find("\"list\":") + 8;
	// find the matching end
	int n = 0;
	for (stop=start; stop<content.length(); stop++) {
		if (content[stop] == '[') {
			n++;
		} else if (content[stop] == ']' && --n == 0) {
			break;
		}
	}
	std::string list = content.substr(start, stop-start+1);
	const char* list_str = list.c_str();

	// 3. json parse
	jsmn_parser parser;
	int nbtokens = jsonGetNrTokens(&parser, list_str, list.length());
	if (nbtokens <= 0) {
		my_printf("list json parsing error: %s\n", list_str);
		return statements;
	}

	jsmntok_t tokens[nbtokens];
	jsonReadTokens(&parser, list_str, list.length(), tokens, nbtokens);

	// 4. process the batch_info
	if (tokens[0].type != JSMN_ARRAY) {
		my_printf("list json parsing error: json object is not an array: [%s]\n", list_str);
		return statements;
	}

	// sometimes doing something stupid like that works
	// better than something more clever
	int done = 0;
	int size = 0;
	std::string path = "";
	std::string blocklist = "";
	for (int i=0; i<nbtokens; i++) {
		jsmntok_t *t = &tokens[i];

		std::string key = jsonGetString(t, list_str);
		if (key == "blocklist") {
			blocklist = jsonGetString(t+1, list_str);
			done++;
			i++;
		} else if (key == "path") {
			path = jsonGetString(t+1, list_str);
			done++;
			i++;
		} else if (key == "size") {
			size = jsonGetInt(t+1, list_str);
			done++;
			i++;
		}

		if (done == 3) {
			std::string statement = "\"8796754824148\", \"" + host_key + "\", \"" + path + "\", " + to_str(size) + ", \"" + blocklist + "\"";
			statements.push_back(statement);
			done = 0;
		}
	}

	// insert an empty message if the list in the list message is empty
	if (statements.size() == 0) {
		std::string statement = "\"8796754824148\", \"" + host_key + "\", \"\", " + to_str(-1) + ", \"\"";
	}

	return statements;
}

void processEventPair(Event* evReq, Event* evRsp, insert_statement_fn insert_stmt_fn) {
	std::vector<std::string> statements;
	int table = 0;

	if (evReq->paramContains(HTTP, HTTP_HEADER_REQUEST, "/list")) {
		statements = parse_list(evReq->getParam(HTTP, HTTP_HEADER_PAYLOAD)->getValueBytes(), evRsp->getParam(HTTP, HTTP_HEADER_PAYLOAD)->getValueBytes());
		table = 0;
	} else if (evReq->paramContains(HTTP, HTTP_HEADER_REQUEST, "/commit_batch")) {
		statements = parse_commit_batch(evReq->getParam(HTTP, HTTP_HEADER_PAYLOAD)->getValueBytes());
		table = 1;
	}

	if (statements.size() == 0) {
		return;
	}

	__atomic_fetch_add(&monotonic_counter, 1, __ATOMIC_RELAXED);

	for (unsigned int i=0; i<statements.size(); i++) {
		std::string result = to_str(monotonic_counter) + ", " + statements[i];
		insert_stmt_fn(table, result.c_str());
	}
}

void libseal_process_log_at_runtime(char *req, char *rsp, unsigned int req_len, unsigned int rsp_len, insert_statement_fn insert_stmt_fn) {
	//We first check if the request is a message we want to analyze to save cpu cycles
	//as httpToEvent analyses the entire message
	//if (strstr(req, "POST /commit_batch") || strstr(req, "POST /list")) {
		Event evReq;
		Event evRsp;

		httpToEvent(&evReq, req, req_len);
		httpToEvent(&evRsp, rsp, rsp_len);

		processEventPair(&evReq, &evRsp, insert_stmt_fn);
	//}
}

const char *tables[] = { "list", "commit_batch", 0 };
const char **get_tables() {
	return tables;
}

char *libseal_init_relations() {
	std::string result = "\
		CREATE TABLE `list` ( \
		`time`	INTEGER NOT NULL, \
		`account`	TEXT NOT NULL, \
		`host_key`	TEXT NOT NULL, \
		`path`	TEXT NOT NULL, \
		`size`	INTEGER NOT NULL, \
		`blocklist`	TEXT NOT NULL, \
		PRIMARY KEY(`time`,`account`,`host_key`, `path`) \
	); \
		CREATE TABLE `commit_batch` ( \
		`time`	INTEGER NOT NULL, \
		`account`	TEXT NOT NULL, \
		`host_key`	TEXT NOT NULL, \
		`path`	TEXT NOT NULL, \
		`size`	INTEGER NOT NULL, \
		`blocklist`	TEXT NOT NULL, \
		PRIMARY KEY(`time`,`account`,`host_key`, `path`) \
	);";

	char *s = (char*) malloc((result.length() + 1)*sizeof(*s));
	memcpy(s, result.c_str(), result.length());
	s[result.length()] = '\0';
	return s;
}

void libseal_do_audit(execute_stmt_fn execute_stmt_fn) {
	// this is for dropbox
	execute_stmt_fn(string("\
				SELECT c1.path, c1.account, c1.time, c2.time \
				FROM commit_batch c1 \
				LEFT JOIN (SELECT MIN(time) AS time, path, account \
					FROM commit_batch \
					WHERE size = -1 \
					GROUP BY path, account) c2 \
				ON c2.path = c1.path \
				AND c2.account = c1.account \
				AND c2.time > c1.time \
				WHERE c1.size >= 0 \
				AND EXISTS (SELECT DISTINCT host_key FROM list l1 \
					WHERE time > c1.time \
					AND (time < c2.time OR c2.time IS NULL) \
					AND account = c1.account \
					AND NOT EXISTS (SELECT 1 FROM list l2 \
						WHERE time > c1.time \
						AND (time < c2.time OR c2.time IS NULL)  \
						AND l1.host_key = host_key AND c1.path = path)); \
				").c_str());

	execute_stmt_fn(string(" \
				SELECT c.time, c.path, l.account \
				FROM commit_batch c \
				JOIN list l \
				ON l.time < c.time AND l.path = c.path \
				AND l.account = c.account \
				WHERE l.blocklist!= c.blocklist AND l.size != -1 AND l.size != -1 AND NOT \
				EXISTS (SELECT 1 FROM commit_batch WHERE account = l.account AND path = l.path AND time < l.time AND time > c.time AND size != -1); \
				").c_str());
}

void libseal_do_trimming(execute_stmt_fn execute_stmt_fn) {
	//TODO
	return;
}
