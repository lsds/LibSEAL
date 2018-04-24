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

#include <stdlib.h>
#include <string>

#include "json_helper.h"
#include "util.h"

int jsonGetInt(jsmntok_t *token, const char *start) {
	return (token->type == JSMN_PRIMITIVE)
			? atoi(start + token->start)
			: -1;
}

long long jsonGetLongLong(jsmntok_t *token, const char *start) {
	return (token->type == JSMN_PRIMITIVE)
			? atoll(start + token->start)
			: -1;
}

string jsonGetString(jsmntok_t *token, const char *start) {
	if (token->type == JSMN_STRING) {
		return std::string(start + token->start, token->end - token->start);
	}
	else if (token->type == JSMN_PRIMITIVE) {
		return to_str(jsonGetLongLong(token, start));
	}

	return "";
}

int jsonGetNrTokens(jsmn_parser *parser, const char *json, int length) {
	jsmn_init(parser);
	return jsmn_parse(parser, json, length, NULL, 0);
}

void jsonReadTokens(jsmn_parser *parser, const char *json, int length, jsmntok_t *tokens, int nrOfTokens) {
	jsmn_init(parser);
	jsmn_parse(parser, json, length, tokens, nrOfTokens);
}

void skipToNextOptypeObject(jsmntok_t *tokens, size_t *pos, const char *payload, std::string optype) {
	while (tokens[*pos].type != JSMN_OBJECT
			|| tokens[(*pos)+1].type != JSMN_STRING
			|| jsonGetString(&tokens[(*pos)+1], payload) != optype) {
		(*pos)++;
	}
}

