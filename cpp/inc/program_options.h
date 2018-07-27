#pragma once
#include <string>
#include <map>

/// Option types
typedef std::pair<char, std::string> TOpt;	
typedef std::map<char, std::string> TOptMap;

/// Option class for simple option parsing and holding in map 
class ProgramOptions {
private:
	TOptMap mOptMap;
	int mOptInd;	// index to actual argv when parsing 
    const char* mOptArg;	// option's argument, if extended option
	const char mEof;

	/* Opt::getOpt() parses argv similar to POSIX getopt()
		option delimiter: '-'
		optstr decribes all allowed options: 
			"x"  = simple option, no optional argument
			"x:" = extended option, with optional argument
		returns 0, if argv is not an allowed argument in optstr
		returns -1 after last argument has been parsed
	*/
    char getOpt(int argc, const char* argv[], const char *optstr);

public:
    ProgramOptions(int ac, const char* av[], const char* optDesc);

	bool exists(char option);

	std::string& getOptArg(char option);
	
friend int mapSize_test(ProgramOptions& opts);

};