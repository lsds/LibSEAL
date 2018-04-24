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

#include "git.h"

extern "C" {
#ifdef COMPILE_WITH_INTEL_SGX
extern int my_printf(const char *format, ...);
#endif
}

void addRef(map<string,string> *untrustedRefs, string ref) {
	(*untrustedRefs)[ref.substr(41,string::npos)] = ref.substr(0,40);
}

char *initOtherRefs(char *ptr, map<string,string> *refs) {
	int length;

	while ((length = gitReadLengthField(ptr)) > 0) {
		addRef(refs, string((char *) ptr+4, length - 5));
		ptr += length;
	}

	return ptr;
}

char *initFirstRef(char *ptr, map<string,string> *untrustedRefs) {
	int length = gitReadLengthField(ptr);
	string ref;

	if (length != 0) {
		if ((ref = string((char *) ptr+4)) != GIT_NO_REF_LINE) {
			// we found a real ref
			addRef(untrustedRefs, ref);
		}
		// else: no refs available.
	}

	return ptr + length;
}

char *initRefs(char *ptr, map<string,string> *untrustedRefs) {
	ptr = initFirstRef(ptr, untrustedRefs);
	return initOtherRefs(ptr, untrustedRefs);
}

void parseGitPackAdvertisement(Event *result, map<string,string> *untrustedRefs) {
	size_t bytesLength;
	char *ptr;

	if ((ptr = getHttpPayload(result, &bytesLength)) == (char*) 0) {
		return;
	}

	// skip service description
	ptr += gitReadLengthField(ptr);

	// We always encounter a 0000 length field here.
//	if (gitReadLengthField(ptr) != 0) {
//		my_printf("Unhandled case. The value of this length field was: %d\n", gitReadLengthField(ptr));
//	}
	ptr += 4;

	initRefs(ptr, untrustedRefs);
}

void addEventParams(Event *request, Event *result, string repository, map<string,string>* refs) {
	string refStr;
	for (map<string,string>::const_iterator it = refs->begin(); it != refs->end(); it++) {
		refStr += it->first + " " + it->second + ",";
	}

	request->add(new Param(GIT, GIT_WHAT, GIT_WHAT_ADVREQ));
	request->add(new Param(GIT, GIT_SERVICE, GIT_SERVICE_ADVERTISE_PACK));
	request->add(new Param(GIT, GIT_REPOSITORY, repository));

	result->add(new Param(GIT, GIT_WHAT, GIT_WHAT_ADVRSP));
	result->add(new Param(GIT, GIT_REFS, refStr));
}

void transformGitPackAdvertisement(Event *request, Event *result) {
	string repository;
	map<string,string> untrustedRefs;

	repository = gitGetRepository(request);
	parseGitPackAdvertisement(result, &untrustedRefs);

	addEventParams(request, result, repository, &untrustedRefs);
}
