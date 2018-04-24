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

#ifndef AUDITORS_OWNCLOUD_INCLUDE_OWNCLOUD_MAIN_H_
#define AUDITORS_OWNCLOUD_INCLUDE_OWNCLOUD_MAIN_H_

#include "auditing_interface_c.h"
#include "http.h"
#include "jsmn.h"
#include "json_helper.h"

/* String Constants */

extern string OWNCLOUD_ANNOTATION_START;
extern string OWNCLOUD_ANNOTATION_END;

extern string OWNCLOUD_ADDANNOTATION;
extern string OWNCLOUD_ADDED;
extern string OWNCLOUD_ADDMEMBER;
extern string OWNCLOUD_ARGS;
extern string OWNCLOUD_CLIENT_OPS;
extern string OWNCLOUD_COMMAND;
extern string OWNCLOUD_CONFLICT;
extern string OWNCLOUD_ESID;
extern string OWNCLOUD_FILEID;
extern string OWNCLOUD_INSERTTEXT;
extern string OWNCLOUD_LENGTH;
extern string OWNCLOUD_MEMBERID;
extern string OWNCLOUD_MEMBER_ID;
extern string OWNCLOUD_NAME;
extern string OWNCLOUD_NEWOPS;
extern string OWNCLOUD_OPTYPE;
extern string OWNCLOUD_OPS;
extern string OWNCLOUD_POSITION;
extern string OWNCLOUD_REMOVEANNOTATION;
extern string OWNCLOUD_REMOVETEXT;
extern string OWNCLOUD_RESULT;
extern string OWNCLOUD_SOURCEPARAGRAPHPOSITION;
extern string OWNCLOUD_SPLITPARAGRAPH;
extern string OWNCLOUD_STATUS;
extern string OWNCLOUD_SYNCOPS;
extern string OWNCLOUD_TEXT;
extern string OWNCLOUD_TIMESTAMP;
extern string OWNCLOUD_UPDATE;

extern string OWNCLOUD_CMD_DOCS_CREATE;
extern string OWNCLOUD_CMD_DOCS_JOIN;
extern string OWNCLOUD_CMD_DOCS_POLL;
extern string OWNCLOUD_CMD_DOCS_JSON_SYNCOPS;

extern string OWNCLOUD_WHAT;
extern string OWNCLOUD_WHAT_CREATE;
extern string OWNCLOUD_WHAT_JOIN;
extern string OWNCLOUD_WHAT_POLL;

/* Docs Application*/

bool docsIsCreate(Event *ev);
bool docsIsJoin (Event *ev);
bool docsIsPoll(Event *ev);

void docsCreate(Event *response);
void docsJoin(Event *request, Event *response);
void docsPoll(Event *request, Event *response);


/* Misc */

bool owncloudIsJson(Event *ev);
bool owncloudIsOK(Event *ev);

#endif /* AUDITORS_OWNCLOUD_INCLUDE_OWNCLOUD_MAIN_H_ */
