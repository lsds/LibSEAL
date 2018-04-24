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

#ifndef LOGGER_H_
#define LOGGER_H_


#include <string>
#include <string.h>
#include <iostream>
#include <fstream>
#include <istream>
#include <vector>

#include "events.h"
#include "http.h"
#include "param.h"

using namespace std;

class Logger {
	private:
		fstream input;

	public:
		Logger(string fifo);
		bool getNext(Event *req, Event *rsp);
};


#endif /* LOGGER_H_ */

