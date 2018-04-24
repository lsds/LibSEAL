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

extern "C" {
#ifdef COMPILE_WITH_INTEL_SGX
extern int my_printf(const char *format, ...);
#endif
}

void addCommand(map<string,string> *commands, string cmd) {
	(*commands)[cmd.substr(82,string::npos)] = cmd.substr(0,81);
}

char *initFirstCommand(char *ptr, map<string,string> *commands) {
	int length = gitReadLengthField(ptr);

	if (length > 0) {
		addCommand(commands, string((char *) ptr+4));
		ptr += length;
	}

	return ptr;
}

char *initOtherCommands(char *ptr, map<string,string> *commands) {
	int length;

	while ((length = gitReadLengthField(ptr)) > 0) {
		addCommand(commands, string((char *) ptr+4, length - 4));
		ptr += length;
	}

	return ptr;
}

char *initCommands(char *ptr, map<string,string> *commands) {
	ptr = initFirstCommand(ptr, commands);
	return initOtherCommands(ptr, commands);
}

void parseGitReceivePackRequest(Event *request, map<string,string> *commands) {
	char *bytesStart;
	size_t bytesLength;
	char *ptr;

	if ((bytesStart = getHttpPayload(request, &bytesLength)) == (char*)0) {
		return;
	}

	ptr = initCommands(bytesStart, commands);

	// Read zero length field
//	if (gitReadLengthField(ptr) != 0) {
//		my_printf("Unknown case. Length field was not 0.\n");
//	}
	ptr += 4;

	/**
	 * TODO: Handle the case of multiple deletes (no creates/updates!). Supposedly no PACK present.
	 */
}

void initResults(char *ptr, map<string,bool> *results) {
	int length;
	int status;
	string refname;

	while ((length = gitReadLengthField(ptr)) > 0) {
		status = gitIsOk(ptr+4);
		if (status) {
			refname = string((char*) ptr+7).substr(0,length-8);
		}
		else {
			string rest = string((char*) ptr+7).substr(0,length-8);
			int pos = rest.find(' ');
			refname = rest.substr(0, pos);
		}
		(*results)[refname] = status;
		ptr += length;
	}
}

bool parseGitReceivePackResult(Event *result, map<string,bool> *results) {
	Param *payload;
	size_t bytesLength;
	char *ptr;
	bool unpackStatus;

	payload = result->getParam(HTTP, HTTP_HEADER_PAYLOAD);
	if ((payload == (Param*)0) || (bytesLength = payload->getValueBytesLength()) <= 4) {
		return false;
	}

	// Skip information such as "Resolving deltas .... "
	ptr = payload->getValueBytes();
	while (*(ptr + 4) != 0x01) {
		ptr += gitReadLengthField(ptr);
	}
	ptr += 5;

	unpackStatus = gitIsOk(ptr+11);

	ptr += gitReadLengthField(ptr);

	initResults(ptr, results);

	return unpackStatus;
}

void addEventParams(Event *request, Event *result, string repository, bool unpackStatus, map<string,string> *commands, map<string,bool> *results) {
	string strCommands;

	for (map<string,string>::const_iterator it = commands->begin(); it != commands->end(); it++) {
		strCommands += it->first + " " + it->second + " " + ((*results)[it->first] ? "ok" : "ng") + ",";
	}

	request->add(new Param(GIT, GIT_WHAT, GIT_WHAT_RCVREQ));
	request->add(new Param(GIT, GIT_SERVICE, GIT_SERVICE_RECEIVE_PACK));
	request->add(new Param(GIT, GIT_REPOSITORY, repository));
	request->add(new Param(GIT, GIT_COMMANDS, strCommands));

	result->add(new Param(GIT, GIT_WHAT, GIT_WHAT_RCVRSP));
	result->add(new Param(GIT, GIT_UNPACK_STATUS, unpackStatus ? "ok" : "ng"));
}

void transformGitReceivePackRequestResult(Event *request, Event *result) {
	string repository;
	bool unpackStatus;

	map<string,string> reqCommands; // refname -> command
	map<string,bool> resResults; // refname -> status

	unpackStatus = parseGitReceivePackResult(result, &resResults);
	repository = gitGetRepository(request);
	parseGitReceivePackRequest(request, &reqCommands);

	addEventParams(request, result, repository, unpackStatus, &reqCommands, &resResults);
}
