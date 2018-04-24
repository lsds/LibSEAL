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

#ifndef GIT_H_
#define GIT_H_

#include <string>
#include <map>
#include <vector>

#include "events.h"
#include "http.h"
#include "util.h"

using namespace std;

extern string gitZeroId;

extern string GIT_BRANCH_MASTER;
extern string GIT_NO_REF_LINE;
extern string GIT_STATE_VAR_INT_REFCOUNT;
extern string GIT_SYMREF_HEAD;

extern string GIT_COMMANDS;
extern string GIT_REFS;
extern string GIT_REPOSITORY;
extern string GIT_SERVICE;
extern string GIT_SERVICE_ADVERTISE_PACK;
extern string GIT_SERVICE_RECEIVE_PACK;
extern string GIT_STATUS;
extern string GIT_UNPACK_STATUS;

extern string GIT_WHAT;
extern string GIT_WHAT_ADVREQ;
extern string GIT_WHAT_ADVRSP;
extern string GIT_WHAT_RCVREQ;
extern string GIT_WHAT_RCVRSP;

bool gitIsAck(char *ptr);
bool gitIsDone(char *ptr);
bool gitIsNak(char *ptr);
bool gitIsOk(char *ptr);
bool gitIsPack(char *ptr);
bool gitIsZeroRef(string ref);

unsigned int gitReadLengthField(char *ptr);

char *gitSkipInfotext(char *ptr);

bool gitIsReceivePack(Event *event);
bool gitIsReceivePackAdvertisement(Event *event);
bool gitIsReceivePackRequest(Event *event);
bool gitIsReceivePackResult(Event *event);

bool gitIsUploadPack(Event *event);
bool gitIsUploadPackAdvertisement(Event *event);

string gitGetRepository(Event *event);

void transformGitPackAdvertisement(Event *request, Event *result);
void transformGitReceivePackRequestResult(Event *request, Event *result);

void crudCommandsGitPackAdvertisement(vector<string> *commands, Event *request, Event *result);
void crudCommandsGitReceivePackRequestResult(vector<string> *commands, Event *request, Event *result);

#endif /* GIT_H_ */
