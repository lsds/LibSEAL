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

#include "param.h"

using namespace std;

void Param::init(Protocol protocol, string name, ParamType type) {
	this->name = name;
	this->type = type;
	this->protocol = protocol;
	this->value_bytes = (char *) 0;
}

Param::Param(Protocol protocol, string name, string value) {
	init(protocol, name, STRING);
	this->value_string = value;
}

Param::Param(Protocol protocol, string name, char *bytes, int length) {
	init(protocol, name, BYTES);
	this->value_bytes = bytes;
	this->value_bytes_length = length;
}

Param::Param(Protocol protocol, string name, int i) {
	init(protocol, name, INT);
	this->value_int = i;
}

Param::Param(Protocol protocol, string name, bool b) {
	init(protocol, name, BOOL);
	this->value_bool = b;
}

Param::~Param() {
	if (this->value_bytes != (char *) 0) {
		free(this->value_bytes);
	}
}

string Param::getName() {
	return name;
}

Protocol Param::getProtocol() {
	return protocol;
}

bool Param::hasName(string name) {
	return this->name == name;
}

ParamType Param::getType() {
	return this->type;
}

string Param::getValueString() {
	return this->value_string;
}

bool Param::getValueBool() {
	return this->value_bool;
}

int Param::getValueInt() {
	return this->value_int;
}

char *Param::getValueBytes() {
	return this->value_bytes;
}

int Param::getValueBytesLength() {
	return this->value_bytes_length;
}

string Param::toString() {
	string s;
	s = "Param(name=" + this->name;
	s += ", protocol="; 
	s += protocolStrings[this->protocol] + ", type=";

	switch (this->type) {
		case BOOL:
			s += "bool, value=" + this->value_bool;
			break;
		case INT:
			s += "int, value=" + this->value_int;
			break;
		case STRING:
			s += "string, value=" + this->value_string;
			break;
		case BYTES:
			s += "bytes, length=";
			s += this->value_bytes_length + ", value=[raw bytes: hex(";
			if (this->value_bytes_length <= 10) {
				for (int i = 0; i < this->value_bytes_length; i++) {
					s += " " + toHexStr(this->value_bytes[i]);
				}
			}
			else {
				for (int i = 0; i < 5; i++) {
					s += " " + toHexStr(this->value_bytes[i]);
				}
				s + " ...";
				for (int i = this->value_bytes_length-5; i < this->value_bytes_length; i++) {
					s += " " + toHexStr(this->value_bytes[i]);
				}
			}
			s += " )]";
			break;
	}

	s += ")";

	return s;
}
