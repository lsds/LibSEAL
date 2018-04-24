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

#ifndef PARAM_H_
#define PARAM_H_

#include <string>
#include <stdlib.h>
#include <string.h>
#include "generic.h"
#include "util.h"

using namespace std;

enum ParamType {
	STRING = 0,
	BYTES = 1,
	INT = 2,
	BOOL = 3
};

class Param {

	private:
		ParamType type;
		string name;
		char *value_bytes;
		int value_bytes_length;
		int value_int;
		bool value_bool;
		string value_string;
		Protocol protocol;
		void init(Protocol protocol, string name, ParamType type);

	public:
		Param(Protocol protocol, string name, string value);
		Param(Protocol protocol, string name, char *bytes, int length);
		Param(Protocol protocol, string name, int i);
		Param(Protocol protocol, string name, bool b);
		~Param();

		string getName();
		Protocol getProtocol();
		bool hasName(string name);
		ParamType getType();
		string getValueString();
		bool getValueBool();
		int getValueInt();
		char *getValueBytes();
		int getValueBytesLength();
		string toString();
};


#endif /* PARAM_H_ */
