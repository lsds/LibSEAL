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

#include "http.h"

extern "C" {
#ifdef COMPILE_WITH_INTEL_SGX
extern int my_printf(const char *format, ...);
extern void ocall_exit(int s);
#else
#include <stdio.h>
#include <stdlib.h>
#define ocall_exit(s) exit(s)
#endif
}

string HTTP_HEADER_CONNECTION = "Connection";
string HTTP_HEADER_CONTENT_LENGTH = "Content-Length";
string HTTP_HEADER_CONTENT_TYPE = "Content-Type";
string HTTP_HEADER_PAYLOAD = "Payload";
string HTTP_HEADER_REQUEST = "Request";
string HTTP_HEADER_RESPONSE = "Response";
string HTTP_HEADER_TRANSFER_ENCODING = "Transfer-Encoding";
string HTTP_HEADER_STATUS = "Status";

bool isEmptyline(string line) {
	return line.empty() || (line.size() <= 2 && (line[0] == '\r' || line[0] == '\n'));
}

/**
 * Creates a new Event and corresponding Params
 * from the provided input stream this->input.
 */
bool readHttpMessage(Event *e, string input) {
	string line;
	int contentLength = -1;
	int headerCount = 0;
	bool isChunked = false;
//	bool connectionClose = false;
	char *payload = (char*) 0;
	size_t pos = 0;

	line = input.substr(pos, input.find('\n', pos) - pos); 
	while (line.length() > 0) {
		if (isEmptyline(line)) {
			if (headerCount == 0) {
				// Empty line that does not belong to any packet
				continue;
			}
			else {
				// Empty line found after headers. Done reading headers.
				break;
			}
		}
		else {
			if (headerCount == 0) {
				// First line of the HTTP Message.
				// This either is the HTTP Request line or Status-Code (upon Response) line.

				string name;
				if (isHttpRequest(line)) {
					name = HTTP_HEADER_REQUEST;
				}
				else if (isHttpResponse(line)) {
					name = HTTP_HEADER_RESPONSE;
				}
				else {
#ifdef COMPILE_WITH_INTEL_SGX
					my_printf("Unimplemented case: %s\n", line.c_str());
#else
					//printf("Unimplemented case: %s\n", line.c_str());
#endif
					ocall_exit(1);
				}

				e->add(new Param(HTTP, name, line.substr(0,line.length()-1)));
				headerCount++;
			}
			else {
				if (line.find(HTTP_HEADER_CONTENT_LENGTH) == 0) {
					// We are handling the Content-Length header.
					contentLength = atoi(line.substr(HTTP_HEADER_CONTENT_LENGTH.length()+2, string::npos).c_str());
				}
				else if (line.find(HTTP_HEADER_TRANSFER_ENCODING) == 0
						&& line.substr(HTTP_HEADER_TRANSFER_ENCODING.length()+2, string::npos).find("chunked") == 0) {
					// We are handling the Transfer-Encoding header.
					// This is interesting for chunked messages.
					isChunked = true;
				}
/*
				else if (line.find(HTTP_HEADER_CONNECTION) == 0
						&& line.substr(HTTP_HEADER_CONNECTION.length()+2, string::npos).find("close") == 0) {
					// Connection: close
					connectionClose = true;
				}
*/

				size_t colon = line.find(":");
				if (colon != string::npos) {
					string value = line.substr(colon+2,line.size()-colon-3);
					e->add(new Param(HTTP, line.substr(0,colon), value));
					headerCount++;
				}
			}
		}

		pos += line.length() + 1;
		line = input.substr(pos, input.find('\n', pos) - pos);
	}

	/*
	 * Done reading headers.
	 * If:
	 *     Content-Length is > 0,
	 *  OR Transfer-Encoding: Chunked
	 *  OR Connection: close
	 * Then:
	 *     Read the payload
	 */

	if (isChunked) {
		pos += 2; // skip empty line
		contentLength = 0;
		payload = readHttpChunkedMessage(input, &pos, &contentLength);
	}
	else if (contentLength > 0) {
		pos += 2; // skip empty line
		payload = (char *) malloc(sizeof(*payload) * (contentLength + 1));
		memcpy(payload, input.substr(pos, contentLength).c_str(), contentLength);
		payload[contentLength - 1] = '\0';
	}

	if (e->paramCnt() > 0 && contentLength > 0) {
		e->add(new Param(HTTP, HTTP_HEADER_CONTENT_LENGTH, contentLength));
		if (payload != (void*) 0) {
			e->add(new Param(HTTP, HTTP_HEADER_PAYLOAD, payload, contentLength));
		}
	}

	return e->paramCnt() > 1;
}

unsigned int readHttpChunkSize(string stream, size_t *pos) {
	char strChunkSize[8];
	char c;
	int bytesRead = 0;

	while (isHex((c = stream.at(*pos + bytesRead)))) {
		strChunkSize[bytesRead] = c;
		bytesRead++;
	}

	*pos += bytesRead;

	return hexToInt(strChunkSize, bytesRead);
}


/**
 * Reads an Http-chunked message from input.
 * The length of the chunked message is returned in length.
 * The returned pointer must be freed by the caller.
 */
char *readHttpChunkedMessage(string input, size_t *pos, int *length) {
	/*
	 * Inspired by pseudo-code at
	 * http://greenbytes.de/tech/webdav/rfc2616.html#introduction.of.transfer-encoding
	 */
	unsigned int chunkSize = 0;
	char *payload = (char*) 0;
	*length = 0;
	bool cont = true;

	while (cont && (chunkSize = readHttpChunkSize(input, pos)) > 0) {
		*pos += 2; // skip CRLF after chunksize field

		payload = (char *) realloc(payload, sizeof(*payload) * (*length + chunkSize));

		// read payload
		memcpy(payload + *length, input.c_str() + *pos, chunkSize);
		*pos += chunkSize + 2; // advance pointer and skip CRLF

		if (*pos >= input.length()) {
			cont = false;
		}

		*length += chunkSize;
	}

	return payload;
}


bool isHttpRequest(string line) {
	/*
	 * TODO: Implement more sophisticated check
	 * and check for other request methods
	 */
	return line.find("POST") == 0
			|| line.find("GET") == 0
			|| line.find("PUT") == 0
			|| line.find("DELETE") == 0
			|| line.find("PATCH") == 0
			|| line.find("PROPFIND") == 0;
}

bool isHttpResponse(string line) {
	/*
	 * TODO: Implement more sophisticated check.
	 */
	return line.find("HTTP") == 0;
}

bool isUnauthorized(Event *event) {
	return event->paramIs(HTTP, HTTP_HEADER_STATUS, "401 Unauthorized")
			|| event->paramContains(HTTP, HTTP_HEADER_RESPONSE, "401 Unauthorized");
}

char *getHttpPayload(Event *ev, size_t *length) {
	Param *payload;

	payload = ev->getParam(HTTP, HTTP_HEADER_PAYLOAD);
	if ((payload == (void*) 0) || (*length = payload->getValueBytesLength()) <= 4) {
		return (char*) 0;
	}

	return payload->getValueBytes();
}

void httpToEvent(Event *ev, char *msg, int msg_len) {
	readHttpMessage(ev, string(msg, msg_len));
}
