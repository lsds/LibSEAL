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
#include <string.h>

#include "auditing_interface_c.h"
#include "events.h"
#include "git.h"
#include "param.h"
#include "util.h"

#ifdef COMPILE_WITH_INTEL_SGX
extern "C" {
extern int my_printf(const char *format, ...);
}
#endif

#undef SAVE_ALL_REQ_RSP
#ifdef SAVE_ALL_REQ_RSP
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#endif


/*
 * Find (at least some) documentation of the Git protocols at
 *
 * https://github.com/git/git/btatelob/master/Documentation/technical/http-protocol.txt
 * https://github.com/git/git/blob/master/Documentation/technical/pack-protocol.txt
 * https://github.com/git/git/blob/master/Documentation/technical/protocol-common.txt
 *
 */

unsigned long long logictime = 1;
const char *tables[] = { "updates", "advertisements", 0 };

void eventPair2DslForDB(Event *req, Event *rsp, vector<string>& updates, vector<string>& advertisements) {
	int id = __atomic_fetch_add(&logictime, 1, __ATOMIC_RELAXED);

	string repo = req->getParam(GIT, GIT_REPOSITORY)->getValueString();
	string what = req->getParam(GIT, GIT_WHAT)->getValueString();
	string branch;
	size_t f;
	size_t s;

	if (what == GIT_WHAT_ADVREQ) {
		string refs = rsp->getParam(GIT, GIT_REFS)->getValueString();
		string cid;

		while ((f = refs.find(',')) != string::npos) {
			s = refs.find(' ');
			branch = refs.substr(0, s);
			cid = refs.substr(s+1, 40);
			refs = refs.substr(s + 42);
			advertisements.push_back(to_str(id) + ",\"" + repo + "\",\"" + branch + "\",\"" + cid + "\"");
		}
	}
	else if (what == GIT_WHAT_RCVREQ) {
		string commands = req->getParam(GIT, GIT_COMMANDS)->getValueString();
		string ref1;
		string ref2;
		string result;
		char type;

		while ((f = commands.find(',')) != string::npos) {
			s = commands.find(' ');
			branch = commands.substr(0, s);
			ref1 = commands.substr(s+1, 40);
			ref2 = commands.substr(s+42, 40);
			result = commands.substr(s+83, 2);

			if (ref2 == gitZeroId) {
				type = 'd';
			}
			else if (ref1 == gitZeroId) {
				type = 'c';
			}
			else {
				type = 'u';
			}

			commands = commands.substr(s + 86);

			updates.push_back(to_str(id) + ",\"" + repo + "\",\"" + branch + "\",\"" + ref2 + "\",\"" + type + "\"," + to_str((result == "ok" ? 1 : 0)));
		}
	}
}

const char **get_tables() {
	return tables;
}

#ifdef SAVE_ALL_REQ_RSP
static unsigned long commit_id = 0;

static void write_to_file(unsigned long cid, char* msg, unsigned int len, int isreq) {
	char filename[256];
	sprintf(filename, "push_%s_%lu", (isreq?"req":"rsp"), cid);

	int fd = open(filename, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR);
	if (fd == -1) {
		printf("Error opening file %s!\n", filename);
		return;
	}

	unsigned int ret = 0;
	while (ret < len) {
		int r = write(fd, msg + ret, len - ret);
		if (r > 0) {
			ret += r;
		} else {
			printf("Write to %s error %d. Abort!\n", filename, r);
			return;
		}
	}
	close(fd);
}
#endif

void libseal_process_log_at_runtime(char *req, char *rsp, unsigned int req_len, unsigned int rsp_len, insert_statement_fn insert_stmt_fn) {
	Event evReq;
	Event evRsp;
	vector<string> updates;
	vector<string> advertisements;

#ifdef SAVE_ALL_REQ_RSP
	unsigned long cid = __atomic_fetch_add(&commit_id, 1, __ATOMIC_RELAXED);
	write_to_file(cid, req, req_len, 1);
	write_to_file(cid, rsp, rsp_len, 0);
	return;
#endif

	httpToEvent(&evReq, req, req_len);
	httpToEvent(&evRsp, rsp, rsp_len);

	if (gitIsReceivePackRequest(&evReq) && gitIsReceivePackResult(&evRsp)) {
		transformGitReceivePackRequestResult(&evReq, &evRsp);
	}
	else if ((gitIsReceivePack(&evReq) && gitIsReceivePackAdvertisement(&evRsp))
		|| (gitIsUploadPack(&evReq) && gitIsUploadPackAdvertisement(&evRsp))) {
		transformGitPackAdvertisement(&evReq, &evRsp);
	}

	if (evReq.getParam(GIT, GIT_SERVICE) != (Param*)0 || evRsp.getParam(GIT, GIT_SERVICE) != (Param*)0) {
		eventPair2DslForDB(&evReq, &evRsp, updates, advertisements);
	}

	for (unsigned int it=0; it<updates.size(); it++) {
		insert_stmt_fn(0, updates.at(it).c_str());
	}
	for (unsigned int it=0; it<advertisements.size(); it++) {
		insert_stmt_fn(1, advertisements.at(it).c_str());
	}
}

char *libseal_init_relations() {
	string result = "\
						  CREATE TABLE `updates` ( \
								`time`  INTEGER NOT NULL,\
								`repo`  TEXT NOT NULL,\
								`branch`        TEXT NOT NULL,\
								`cid`   TEXT NOT NULL,\
								`type`  TEXT NOT NULL,\
								`result`        INTEGER NOT NULL,\
								PRIMARY KEY(`time`,`repo`,`branch`)\
						  ); \
						  \
						  CREATE TABLE `advertisements` ( \
							   `time`  INTEGER NOT NULL, \
							   `repo`  TEXT NOT NULL, \
							   `branch`        TEXT NOT NULL, \
							   `cid`   TEXT NOT NULL, \
							   PRIMARY KEY(`time`,`repo`,`branch`) \
						  ); \
						  CREATE TABLE `branchmods` ( \
								  `time`  INTEGER NOT NULL,\
								  `repo`  TEXT NOT NULL,\
								  `branch`        TEXT NOT NULL,\
								  `cid`   TEXT NOT NULL,\
								  `type`  TEXT NOT NULL,\
								  `result`        INTEGER NOT NULL,\
								  PRIMARY KEY(`time`,`repo`,`branch`)\
								  ); \
						  \
						  CREATE VIEW `branchcnt2` AS \
						  SELECT DISTINCT a.`time`,a.`repo`,COUNT(bm.`branch`) AS count\
						  FROM advertisements a \
						  JOIN branchmods bm \
						  ON bm.time < a.time AND bm.`repo` = a.`repo` \
						  WHERE bm.`type` != 'd' AND NOT EXISTS (SELECT 1 FROM branchmods WHERE `branch` = bm.`branch` AND `repo` = bm.`repo` \
								  AND `time` < a.`time` AND `time` > bm.`time`) GROUP BY a.`time`,a.`repo`,a.`branch`; \
						  \
						  CREATE VIEW `incomplete2` AS \
								SELECT `time`,`repo` \
						  		FROM advertisements a \
						  		NATURAL JOIN branchcnt2 \
						  		WHERE `branch` != 'HEAD' \
						  		GROUP BY `time`,`repo`,`count` HAVING COUNT(`branch`) != `count`; \
					";
	
	char *s = (char*) malloc((result.length() + 1)*sizeof(char *));
	snprintf(s, result.length() + 1, "%s", result.c_str());
	return s;
}

void libseal_do_audit(execute_stmt_fn execute_stmt_fn) {
	//completeness
	execute_stmt_fn(string("SELECT * FROM incomplete2;").c_str());
	//soudness
	execute_stmt_fn(
			string("SELECT * FROM advertisements a WHERE cid <> \
				(     \
						SELECT u.cid \
						FROM updates u \
						WHERE u.repo = a.repo AND \
						u.branch = a.branch AND \
						u.time < a.time \
						ORDER BY u.time DESC \
						LIMIT 1 \
				) \
				;").c_str()
			);
}

void libseal_do_trimming(execute_stmt_fn execute_stmt_fn) {
	execute_stmt_fn(
			string("DELETE \
				FROM updates \
				WHERE time NOT IN (SELECT MAX(time) \
					FROM updates \
					GROUP BY repo, branch) \
				; DELETE FROM advertisements;").c_str()
			);
}
