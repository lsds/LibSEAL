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

string OWNCLOUD_ANNOTATION_START = "^AnnStart;";
string OWNCLOUD_ANNOTATION_END = "^AnnEnd;";

string OWNCLOUD_ADDANNOTATION = "AddAnnotation";
string OWNCLOUD_ADDED = "added";
string OWNCLOUD_ADDMEMBER = "AddMember";
string OWNCLOUD_ARGS = "args";
string OWNCLOUD_CLIENT_OPS = "client_ops";
string OWNCLOUD_COMMAND = "command";
string OWNCLOUD_CONFLICT = "conflict";
string OWNCLOUD_ESID = "es_id";
string OWNCLOUD_FILEID = "fileid";
string OWNCLOUD_INSERTTEXT = "InsertText";
string OWNCLOUD_LENGTH = "length";
string OWNCLOUD_MEMBERID = "memberid";
string OWNCLOUD_MEMBER_ID = "member_id";
string OWNCLOUD_NAME = "name";
string OWNCLOUD_NEWOPS = "new_ops";
string OWNCLOUD_OPTYPE = "optype";
string OWNCLOUD_OPS = "ops";
string OWNCLOUD_POSITION = "position";
string OWNCLOUD_REMOVEANNOTATION = "RemoveAnnotation";
string OWNCLOUD_REMOVETEXT = "RemoveText";
string OWNCLOUD_RESULT = "result";
string OWNCLOUD_SOURCEPARAGRAPHPOSITION = "sourceParagraphPosition";
string OWNCLOUD_SPLITPARAGRAPH = "SplitParagraph";
string OWNCLOUD_STATUS = "status";
string OWNCLOUD_SYNCOPS = "sync_ops";
string OWNCLOUD_TEXT = "text";
string OWNCLOUD_TIMESTAMP = "timestamp";
string OWNCLOUD_UPDATE = "update";

string OWNCLOUD_CMD_DOCS_CREATE = "/apps/documents/ajax/documents/create";
string OWNCLOUD_CMD_DOCS_JOIN = "/apps/documents/session/user/join/";
string OWNCLOUD_CMD_DOCS_POLL = "/apps/documents/session/user/poll";

string OWNCLOUD_WHAT = "what";
string OWNCLOUD_WHAT_CREATE = "create";
string OWNCLOUD_WHAT_JOIN = "join";
string OWNCLOUD_WHAT_POLL = "poll";


bool owncloudIsJson(Event *ev) {
	return ev->paramContains(HTTP, HTTP_HEADER_CONTENT_TYPE, "application/json");
}

bool owncloudIsOK(Event *ev) {
	return ev->paramContains(HTTP, HTTP_HEADER_RESPONSE, "200 OK");
}

