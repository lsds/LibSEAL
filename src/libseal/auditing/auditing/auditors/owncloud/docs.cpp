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

bool docsIsCreate(Event *ev) {
	return ev->paramContains(HTTP, HTTP_HEADER_REQUEST, OWNCLOUD_CMD_DOCS_CREATE);
}

bool docsIsJoin (Event *ev) {
	return ev->paramContains(HTTP, HTTP_HEADER_REQUEST, OWNCLOUD_CMD_DOCS_JOIN);
}

bool docsIsPoll(Event *ev) {
	return ev->paramContains(HTTP, HTTP_HEADER_REQUEST, OWNCLOUD_CMD_DOCS_POLL);
}

string owncloudReadField(Event *event, string field, jsmntok_t *tokens, size_t pos, char *payload) {
	string result;
	if (jsonGetString(&tokens[pos], payload) != field) {
		my_printf("Expecting Owncloud %s field\n", field.c_str());
		result = "";
	}
	else {
		result = jsonGetString(&tokens[pos+1], payload);
		event->add(new Param(OWNCLOUD, field, result));
	}
	return result;
}


/*
 * Add the command(s) that will update the given document doc in correspondence with
 * the _single_ JSON op that is stored at position *pos within *tokens.
 */
string updateDoc_1(jsmntok_t *tokens, size_t *pos, char *payload) {
	string result;

	int pairs = tokens[(*pos)++].size;

	string optype;
	int position;

	optype = jsonGetString(&tokens[(*pos)+1], payload);
	position = jsonGetInt(&tokens[(*pos)+7], payload);

	if (optype == OWNCLOUD_INSERTTEXT) {
		result += "i," + to_str(position) + "," + jsonGetString(&tokens[(*pos)+9], payload);
	}
	else if (optype == OWNCLOUD_REMOVETEXT) {
		result += "r," + to_str(position) + "," + to_str(jsonGetInt(&tokens[(*pos)+9], payload));
	}
	else if (optype == OWNCLOUD_SPLITPARAGRAPH) {
		result += "i," + to_str(position) + "," + "¶";
	}
	else if (optype == OWNCLOUD_ADDANNOTATION) {
		int nameIdx = (jsonGetString(&tokens[(*pos)+8], payload) == OWNCLOUD_LENGTH ? 11 : 9);
		result += "aa," + to_str(position) + "," + jsonGetString(&tokens[(*pos)+nameIdx], payload);
	}
	else if (optype == OWNCLOUD_REMOVEANNOTATION) {
		result += "ar," + to_str(position);
	}

	(*pos) += pairs*2;

	return result.length() != 0 ? "(" + result + ")" : "";
}

/*
 * Create the commands that will update the given doc in correspondence with the JSON ops that are
 * stored within the JSON object at position *pos within *tokens.
 */
string updateDoc(jsmntok_t *tokens, size_t *pos, char *payload) {
	int optypeElements = tokens[*pos].size;

	string result;

	for (int i = 0; i < optypeElements; i++) {
		skipToNextOptypeObject(tokens, pos, payload, OWNCLOUD_OPTYPE);
		result += updateDoc_1(tokens, pos, payload);
	}

	return result;
}


void docsCreate(Event *response) {
	bool status;
	string fileid;
	char *rspPayload;
	size_t rspLength;
	int jsonNrTokens;

	if ((rspPayload = getHttpPayload(response, &rspLength)) == (char*)0) {
		return;
	}

	jsmn_parser parser;
	jsonNrTokens = jsonGetNrTokens(&parser, rspPayload, rspLength);
	jsmntok_t rspTokens[jsonNrTokens];
	jsonReadTokens(&parser, rspPayload, rspLength, rspTokens, jsonNrTokens);

	if (jsonNrTokens < 1 || rspTokens[0].type != JSMN_OBJECT) {
		my_printf("Failed to parse JSON: %d\n", jsonNrTokens);
		return;
	}

	status = owncloudReadField(response, OWNCLOUD_STATUS, rspTokens, 1, rspPayload) == "success";
	fileid = owncloudReadField(response, OWNCLOUD_FILEID, rspTokens, 3, rspPayload);

	response->add(new Param(OWNCLOUD, OWNCLOUD_STATUS, status));
	response->add(new Param(OWNCLOUD, OWNCLOUD_FILEID, fileid));
}

void docsJoin(Event *request, Event *response) {
	string reqLine;
	string fileid;
	char *rspPayload;
	int jsonNrTokens;
	size_t rspLength;
	bool status = false;
	string esid;
	string memberid;

	if ((rspPayload = getHttpPayload(response, &rspLength)) == (char*)0) {
		return;
	}

	// Extract the file id from the first line
	reqLine = request->getParam(HTTP, HTTP_HEADER_REQUEST)->getValueString();
	fileid = reqLine.substr(reqLine.find(OWNCLOUD_CMD_DOCS_JOIN) + OWNCLOUD_CMD_DOCS_JOIN.length());
	fileid = fileid.substr(0, fileid.find(' '));


	jsmn_parser parser;
	jsonNrTokens = jsonGetNrTokens(&parser, rspPayload, rspLength);
	jsmntok_t rspTokens[jsonNrTokens];
	jsonReadTokens(&parser, rspPayload, rspLength, rspTokens, jsonNrTokens);

	if (jsonNrTokens < 1 || rspTokens[0].type != JSMN_OBJECT) {
		my_printf("Failed to parse JSON: %d\n", jsonNrTokens);
		return;
	}

	esid = owncloudReadField(response, OWNCLOUD_ESID, rspTokens, 1, rspPayload);
	memberid = owncloudReadField(response, OWNCLOUD_MEMBER_ID, rspTokens, 11, rspPayload);
	status = owncloudReadField(response, OWNCLOUD_STATUS, rspTokens, 17, rspPayload) == "success";

	response->add(new Param(OWNCLOUD, OWNCLOUD_FILEID, fileid));
	response->add(new Param(OWNCLOUD, OWNCLOUD_ESID, esid));
	response->add(new Param(OWNCLOUD, OWNCLOUD_MEMBERID, memberid));
	response->add(new Param(OWNCLOUD, OWNCLOUD_STATUS, status));
}

void docsPoll(Event *request, Event *response) {
	char *reqPayload;
	char *rspPayload;
	size_t reqLength;
	size_t rspLength;
	int reqNrOfTokens;
	int rspNrOfTokens;
	size_t reqPos = 1;
	size_t rspPos = 1;
	string command;
	string esid;
	string memberid;
	string result;
	string update;

	if (((reqPayload = getHttpPayload(request, &reqLength)) == (char*)0)
		|| ((rspPayload = getHttpPayload(response, &rspLength)) == (char*)0)) {
		return;
	}

	jsmn_parser parser;
	reqNrOfTokens = jsonGetNrTokens(&parser, reqPayload, reqLength);
	jsmntok_t reqTokens[reqNrOfTokens];
	jsonReadTokens(&parser, reqPayload, reqLength, reqTokens, reqNrOfTokens);

	rspNrOfTokens = jsonGetNrTokens(&parser, rspPayload, rspLength);
	jsmntok_t rspTokens[rspNrOfTokens];
	jsonReadTokens(&parser, rspPayload, rspLength, rspTokens, rspNrOfTokens);

	if (reqNrOfTokens < 1 || reqTokens[0].type != JSMN_OBJECT) {
		my_printf("Failed to parse request JSON: %d\n",  rspNrOfTokens);
		return;
	}

	if (rspNrOfTokens < 1 || rspTokens[0].type != JSMN_OBJECT) {
		my_printf("Failed to parse response JSON: %d\n", rspNrOfTokens);
		return;
	}

	command = owncloudReadField(request, OWNCLOUD_COMMAND, reqTokens, reqPos, reqPayload);
	request->add(new Param(OWNCLOUD, OWNCLOUD_COMMAND, command));
	reqPos += 2;

	result = owncloudReadField(response, OWNCLOUD_RESULT, rspTokens, rspPos, rspPayload);
	response->add(new Param(OWNCLOUD, OWNCLOUD_RESULT, result));
	rspPos += 2;

	if (command == OWNCLOUD_SYNCOPS) {
		if (jsonGetString(&reqTokens[reqPos++], reqPayload) == OWNCLOUD_ARGS && reqTokens[reqPos++].type == JSMN_OBJECT) {
			esid = owncloudReadField(request, OWNCLOUD_ESID, reqTokens, reqPos, reqPayload);
			request->add(new Param(OWNCLOUD, OWNCLOUD_ESID, esid));
			reqPos += 2;

			memberid = owncloudReadField(request, OWNCLOUD_MEMBER_ID, reqTokens, reqPos, reqPayload);
			request->add(new Param(OWNCLOUD, OWNCLOUD_MEMBERID, memberid));
			reqPos += 2;
		}

		if (result == OWNCLOUD_ADDED) {
			// Client added commands. Search for 'client_ops' in the request and
			// update the state of the central document.
			while (jsonGetString(&reqTokens[reqPos++], reqPayload) != OWNCLOUD_CLIENT_OPS);

			update = updateDoc(reqTokens, &reqPos, reqPayload);
			request->add(new Param(OWNCLOUD, OWNCLOUD_UPDATE, update));
		}
		else if (result == OWNCLOUD_NEWOPS || result == OWNCLOUD_CONFLICT) {
			// Client receives text. Search for 'opt' in the response and
			// update the client's document.
			while (jsonGetString(&rspTokens[rspPos++], rspPayload) != OWNCLOUD_OPS);

			update = updateDoc(rspTokens, &rspPos, rspPayload);
			response->add(new Param(OWNCLOUD, OWNCLOUD_UPDATE, update));
		}
	}
}
