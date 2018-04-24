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

#include "events.h"

#ifdef COMPILE_WITH_INTEL_SGX
extern "C" {
extern int my_printf(const char *format, ...);
}
#endif

Event::Event() {
}

Event::~Event() {
	for (map<string, Param*>::const_iterator it = params.begin(); it != params.end(); it++) {
		delete it->second;
	}
	params.clear();
}

void Event::add(Param *p) {
	params[protocolStrings[p->getProtocol()] + p->getName()] = p;
}

/**
 * Returns this event's parameter with the given name.
 * If no such parameter exists, nullptr is returned.
 */
Param *Event::getParam(Protocol protocol, string name) {
	map<string, Param*>::const_iterator found = params.find(protocolStrings[protocol] + name);

	if (found != params.end()) {
		return found->second;
	}

	return (Param*) 0;
}

bool Event::paramIs(Protocol protocol, string name, string value) {
	Param *p = getParam(protocol, name);
	return (p != (Param*) 0) ? p->getValueString() == value : false;
}

bool Event::paramContains(Protocol protocol, string name, string value) {
	Param *p = getParam(protocol, name);
	return (p != (Param *) 0) ? p->getValueString().find(value) != string::npos : false;
}

string Event::toString() {
	string s = "Event(";
	for (map<string,Param*>::iterator it = params.begin(); it != params.end(); it++) {
		s += it->second->toString() + ", ";
	}
	return s.substr(0,s.length()-2) + ")";
}

int Event::paramCnt() {
	return params.size();
}

string Event::toString(vector<pair<Protocol,string> > *fields) {
	string result = "";
	Param *p;

	for (vector<pair<Protocol,string> >::const_iterator it = fields->begin(); it != fields->end(); it++) {
		p = getParam(it->first, it->second);
		if (p != (Param*)0) {
			result += protocolStrings[it->first] + it->second + ": ";
			switch (p->getType()) {
				case STRING:
					result += p->getValueString();
					break;
				case INT:
					result += to_str(p->getValueInt());
					break;
				case BOOL:
					result += p->getValueBool() ? "1" : "0";
					break;
				case BYTES:
					result += toStringAbbrev(p->getValueBytes(), p->getValueBytesLength(), 20);
					break;
			}
			result += "\n";
		}
	}
	return result.substr(0, result.length()-1);
}
