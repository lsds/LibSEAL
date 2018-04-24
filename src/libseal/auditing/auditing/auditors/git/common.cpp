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

#include "git.h"

#ifdef COMPILE_WITH_INTEL_SGX
extern "C" {
extern int my_printf(const char *format, ...);
}
#endif

unsigned int gitReadLengthField(char *ptr) {
	return hexToInt(ptr, 4);
}

string gitZeroId = "0000000000000000000000000000000000000000";

string GIT_BRANCH_MASTER = "refs/heads/master";
string GIT_NO_REF_LINE = gitZeroId + " capabilities^{}";
string GIT_STATE_VAR_INT_REFCOUNT = "refCount";
string GIT_SYMREF_HEAD = "HEAD";

string GIT_COMMANDS = "Cmds";
string GIT_REFS = "Refs";
string GIT_REPOSITORY = "Repo";
string GIT_SERVICE = "Service";
string GIT_SERVICE_ADVERTISE_PACK = "advertise-pack";
string GIT_SERVICE_RECEIVE_PACK = "receive-pack";
string GIT_STATUS = "Status";
string GIT_UNPACK_STATUS = "UnpackStatus";

string GIT_WHAT = "What";
string GIT_WHAT_ADVREQ = "AdvReq";
string GIT_WHAT_ADVRSP = "AdvRsp";
string GIT_WHAT_RCVREQ = "RcvReq";
string GIT_WHAT_RCVRSP = "RcvRsp";

bool gitIsAck(char *ptr) {
	return (*ptr == 'A') && (*(ptr+1) == 'C') && (*(ptr+2) == 'K');
}

bool gitIsDone(char *ptr) {
	return (*ptr == 'd') && (*(ptr+1) == 'o') && (*(ptr+2) == 'n') && (*(ptr+3) == 'e');
}

bool gitIsNak(char *ptr) {
	return (*ptr == 'N') && (*(ptr+1) == 'A') && (*(ptr+2) == 'K');
}

bool gitIsOk(char *ptr) {
	return *ptr == 'o' && *(ptr+1) == 'k';
}

bool gitIsReceivePack(Event *event) {
	return event->getParam(HTTP, HTTP_HEADER_CONTENT_TYPE) == (Param*)0
			&& event->getParam(HTTP, HTTP_HEADER_PAYLOAD) == (Param*)0
			&& event->getParam(HTTP, HTTP_HEADER_REQUEST)->getValueString().find("service=git-receive-pack") != string::npos;
}

bool gitIsUploadPack(Event *event) {
	return event->getParam(HTTP, HTTP_HEADER_CONTENT_TYPE) == (Param*)0
			&& event->getParam(HTTP, HTTP_HEADER_PAYLOAD) == (Param*)0
			&& event->getParam(HTTP, HTTP_HEADER_REQUEST)->getValueString().find("service=git-upload-pack") != string::npos;
}

bool gitIsReceivePackAdvertisement(Event *event) {
	return event->paramIs(HTTP, HTTP_HEADER_CONTENT_TYPE, "application/x-git-receive-pack-advertisement");
}

bool gitIsReceivePackRequest(Event *event) {
	return event->paramIs(HTTP, HTTP_HEADER_CONTENT_TYPE, "application/x-git-receive-pack-request");
}

bool gitIsReceivePackResult(Event *event) {
	return event->paramIs(HTTP, HTTP_HEADER_CONTENT_TYPE, "application/x-git-receive-pack-result");
}

bool gitIsUploadPackAdvertisement(Event *event) {
	return event->paramIs(HTTP, HTTP_HEADER_CONTENT_TYPE, "application/x-git-upload-pack-advertisement");
}

string gitGetRepository(Event *event) {
	Param *p = event->getParam(HTTP, HTTP_HEADER_REQUEST);
	if (p == (Param*) 0) {
		return "";
	}

	string line = p->getValueString();

	size_t pos = string::npos;
	size_t off = string::npos;

	if (line.find("POST") == 0) {
		off = 5;
		pos = line.find("git-receive-pack");
		if (pos == string::npos) {
			pos = line.find("git-upload-pack");
		}
	}
	else if (line.find("GET") == 0) {
		off = 4;
		pos = line.find("info/refs?service=git-receive-pack");
		if (pos == string::npos) {
			pos = line.find("info/refs?service=git-upload-pack");
		}
	}

	if (pos != string::npos && off != string::npos) {
		return line.substr(off, pos-off);
	}

	return "";
}
