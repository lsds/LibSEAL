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

#ifndef EVENT_H_
#define EVENT_H_

#include <map>
#include <vector>

#include "param.h"
#include "util.h"

using namespace std;

class Event {
	private:
		map<string, Param*> params;

	public:
		Event();
		~Event();
		void add(Param *p);
		int paramCnt();
		Param *getParam(Protocol protocol, string name);
		bool paramIs(Protocol protocol, string name, string value);
		bool paramContains(Protocol protocol, string name, string value);
		string toString();
		string toString(vector<pair<Protocol,string> > *fields);
};

#endif /* EVENT_H_ */
