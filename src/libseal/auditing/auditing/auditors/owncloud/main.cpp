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

#include "owncloud.h"
#include "auditing_interface_c.h"

unsigned long long logictime = 1;
const char *tables[] = { "sync", 0 };

void libseal_process_log_at_runtime(char *req, char *rsp, unsigned int req_len, unsigned int rsp_len, insert_statement_fn insert_stmt_fn) {
	Event evReq;
	Event evRsp;

	httpToEvent(&evReq, req, req_len);
	httpToEvent(&evRsp, rsp, rsp_len);

	unsigned long long id = __atomic_fetch_add(&logictime, 1, __ATOMIC_RELAXED);

	if (docsIsPoll(&evReq) && owncloudIsOK(&evRsp)) {
		evReq.add(new Param(OWNCLOUD, OWNCLOUD_WHAT, OWNCLOUD_WHAT_POLL));
		docsPoll(&evReq, &evRsp);

		if (evReq.paramIs(OWNCLOUD, OWNCLOUD_COMMAND, OWNCLOUD_SYNCOPS)) {
			string res = evRsp.getParam(OWNCLOUD, OWNCLOUD_RESULT)->getValueString();
			string update;
			string from;
			string to;

			if (res == OWNCLOUD_ADDED) {
				from = evReq.getParam(OWNCLOUD, OWNCLOUD_MEMBERID)->getValueString();
				to = "0";
				update = evReq.getParam(OWNCLOUD, OWNCLOUD_UPDATE)->getValueString();
			}
			else if (res == OWNCLOUD_NEWOPS || res == OWNCLOUD_CONFLICT) {
				from = "0";
				to = evReq.getParam(OWNCLOUD, OWNCLOUD_MEMBERID)->getValueString();
				update = evRsp.getParam(OWNCLOUD, OWNCLOUD_UPDATE)->getValueString();
			}

			if (update.length() != 0) {
				std::string sync = to_str_u128(id) + ",\""
						+ evReq.getParam(OWNCLOUD, OWNCLOUD_ESID)->getValueString() + "\","
						+ from + ","
						+ to + ",\""
						+ update + "\"";
				insert_stmt_fn(0, sync.c_str());
			}
		}
	}
}

char *libseal_init_relations() {
	string result = "\
		CREATE TABLE `sync` ( \
		`time`	INTEGER NOT NULL, \
		`esid`	TEXT NOT NULL, \
		`sender`	INTEGER NOT NULL, \
		`receiver`	INTEGER NOT NULL, \
		`upd`	TEXT NOT NULL, \
		PRIMARY KEY(`time`) \
		);\
		\
		CREATE VIEW sound AS \
		SELECT time,esid,sndr,rcvr \
		FROM sync s \
		WHERE sndr=0 AND \
		(SELECT group_concat(upd,'') from (select esid,upd from sync where rcvr = 0  AND time <= s.time AND esid = s.esid order by time) group by esid) \
		NOT LIKE \
		((SELECT group_concat(upd,'') from (select esid,upd from sync where (sndr = s.rcvr OR rcvr = s.rcvr) AND time <= s.time AND esid = s.esid order by time) group by esid) || '%' );";

	char *s = (char*) malloc((result.length() + 1)*sizeof(char *));
	snprintf(s, result.length() + 1, "%s", result.c_str());
	return s;
}

const char **get_tables() {
	return tables;
}

void libseal_do_audit(execute_stmt_fn execute_stmt_fn) {
	execute_stmt_fn(string(" \
				SELECT time,esid,sender,receiver FROM sync o WHERE sender = 0 AND (SELECT group_concat(upd,'') FROM    sync    WHERE esid = o.esid AND time <= o.time    AND receiver = 0 ORDER BY time) NOT LIKE (SELECT group_concat(upd,'') FROM     sync    WHERE esid = o.esid AND time <= o.time    AND sender = o.receiver    OR receiver = o.receiver ORDER BY time) || '%';\
				").c_str());
}

void libseal_do_trimming(execute_stmt_fn execute_stmt_fn) {
	//TODO
	return;
}
