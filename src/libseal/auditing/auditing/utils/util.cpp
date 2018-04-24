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

#include "util.h"

extern "C" {
#ifdef COMPILE_WITH_INTEL_SGX
extern int my_printf(const char *format, ...);
#else
#define my_printf(format, ...) printf(format, ##__VA_ARGS__)
#endif
}


string to_str_u128(unsigned long long i) {
	char buf[64];
	snprintf(buf, 64, "%qu", i);
	return string(buf);
}

string to_str(int i) {
	char buf[64];
	snprintf(buf, 64, "%d", i);
	return string(buf);
}

string toHexStr(char b) {
	unsigned char upper = (unsigned char) b >> 4;
	unsigned char lower = (unsigned char) b % 16;

	string s;
	s += (char)(upper >= 10 ? 'a'+upper-10 : '0'+upper);
	s += (char)(lower >= 10 ? 'a'+lower-10 : '0'+lower);
	return s;
}


string toHexStr(char *b, int length) {
	string s;
	for (int i = 0; i < length; i++) {
		s += toHexStr(b[i]);
	}
	return s;
}

unsigned int hexToInt(char *bytes, int length) {
	unsigned int result = 0;
	int error = -1;

	for (int i = 0; i < length; i++) {
		char offset = 0;

		if (bytes[i] >= 'A' && bytes[i] <= 'F') {
			offset =  - 'A' + 10;
		}
		else if (bytes[i] >= 'a' && bytes[i] <= 'f') {
			offset =  - 'a' + 10;
		}
		else if (bytes[i] >= '0' && bytes[i] <= '9') {
			offset =  - '0';
		}
		else {
			error = i;
			break;
		}

		result += (bytes[i] + offset) << ((length - 1 - i) * 4);
	}

	if (error != -1) {
			//my_printf("Invalid Hex string: %s (%d) on %d [%s]\n", toHexStr(bytes+error,1).c_str(), (int)bytes[error], length, toHexStr(bytes, length));
			return 0xffffffff;
	}

	return result;
}

bool isHex(char b) {
	return (b >= '0' && b <= '9')
			|| (b >= 'a' && b <= 'f')
			|| (b >= 'A' && b <= 'F');
}

string toStringAbbrev(char *bytes, int actualLen, int maxlen) {
	if (maxlen > actualLen) {
		return toHexStr(bytes, actualLen);
	}
	else {
		if (maxlen % 2 == 1) {
			maxlen--;
		}

		string s = "";
		s += toHexStr(bytes, maxlen/2);
		s += " ... << chopped ";
		s += to_str(actualLen - maxlen);
		s += " bytes >> ... ";
		s += toHexStr(bytes + actualLen - maxlen/2, maxlen/2);
		return s;
	}
}
