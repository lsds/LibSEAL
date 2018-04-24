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

#ifndef UTIL_H_
#define UTIL_H_

#include <stdio.h>
#include <string>

using namespace std;

string to_str_u128(unsigned long long i);
string to_str(int i);
string toHexStr(char b);
string toHexStr(char *b, int length);
unsigned int hexToInt(char *b, int length);
bool isHex(char b);
string toStringAbbrev(char *bytes, int actualLen, int maxlen);

#endif /* UTIL_H_ */
