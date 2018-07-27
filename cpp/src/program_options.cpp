#include "stdafx.h"
#include <cstring>
#include "../inc/program_options.h"

char ProgramOptions::getOpt(int argc, const char* argv[], const char* optstr) {
	if (mOptInd >= argc)
		return mEof;
    const char *optch, *colon;
	mOptArg = nullptr;
	if (*argv[mOptInd] == '-') {
		++argv[mOptInd];
		colon = optch = strchr(optstr, *argv[mOptInd]);
		++mOptInd;
		if (optch != nullptr) {
			++colon;
			if (*colon == ':') {
				if (mOptInd < argc) {// else: mOptArg = nullptr = no opt arg
					// check for leading '-'
					if (*argv[mOptInd] != '-') // opt arg starts with '-' = no opt arg
						mOptArg = argv[mOptInd];
				}
			}
			return *optch;
		}
	}
	else {
		++mOptInd; }
	return '\0';
}


ProgramOptions::ProgramOptions(int ac, const char* av[], const char *optDesc) : mEof(-1) {
	mOptInd = 1;
	char c;
    while ((c = getOpt(ac, av, (char*)optDesc)) != mEof) {
		if (c) {
			if (mOptArg)
				mOptMap.insert(TOpt(c, mOptArg));
			else
				mOptMap.insert(TOpt(c, ""));
		}
	}
}

bool ProgramOptions::exists(char option) {
	if (mOptMap.count(option) > 0)
		return true;
	else 
		return false;
}

std::string& ProgramOptions::getOptArg(char option) {
	return mOptMap[option];
}
