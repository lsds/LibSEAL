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

#ifndef HTTP_H_
#define HTTP_H_

#include <string.h>
#include <string>
#include <stdio.h>

#include "events.h"
#include "util.h"

using namespace std;

extern string HTTP_HEADER_CONNECTION;
extern string HTTP_HEADER_CONTENT_LENGTH;
extern string HTTP_HEADER_CONTENT_TYPE;
extern string HTTP_HEADER_PAYLOAD;
extern string HTTP_HEADER_REQUEST;
extern string HTTP_HEADER_RESPONSE;
extern string HTTP_HEADER_TRANSFER_ENCODING;
extern string HTTP_HEADER_STATUS;

bool readHttpMessage(Event *event, string input);

int readHttpChunkSize(string input, size_t pos, int *bytesRead);

/**
 * Reads a chunked HTTP message from input.
 *
 * The function returns a pointer to newly allocated memory
 * holding the message that was read. Parameter *length returns
 * the length of that message.
 */
char *readHttpChunkedMessage(string input, size_t *pos, int *length);

bool isHttpRequest(string line);
bool isHttpResponse(string line);
bool isUnauthorized(Event *event);

char *getHttpPayload(Event *ev, size_t *length);

void httpToEvent(Event *ev, char *msg, int msg_len);


#endif /* HTTP_H_ */
