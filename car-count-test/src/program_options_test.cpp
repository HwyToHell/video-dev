#include "stdafx.h"
#include "../cpp/inc/program_options.h"


using namespace std;


int mapSize_test(ProgramOptions& opts) {
	return opts.mOptMap.size(); 
}

SCENARIO("general option parsing", "[ProgramOptions]")
{
	GIVEN("command line with 1 short and 2 long options") {
		// create argc from argv
        const char* argv_[] = { "progname", "-v", "videoinput", "-c", "-o", "filename", "d" };
		int argc_ = (sizeof(argv_)/sizeof(argv_[0]));		
		ProgramOptions cmdlnOpts(argc_, argv_, "co:v:");
		WHEN("command line is parsed")
		{
			//cmdlnOpts.parseCmdLine(argc_, argv_, "co:v:");
			THEN("arguments are stored in opt_map") {
				REQUIRE(cmdlnOpts.exists('c') == true);
				REQUIRE(cmdlnOpts.exists('o') == true);
				REQUIRE(cmdlnOpts.getOptArg('o') == "filename");
				REQUIRE(cmdlnOpts.exists('v') == true);
				REQUIRE(cmdlnOpts.getOptArg('v') == "videoinput");
			}
			THEN("opt_map has 3 elements") {
				REQUIRE(mapSize_test(cmdlnOpts) == 3);
			}
		}
	}

	GIVEN("command line with no options") {
		// create argc from argv
        const char* argv_[] = { "progname" };
		int argc_ = (sizeof(argv_)/sizeof(argv_[0]));	
		ProgramOptions cmdlnOpts(argc_, argv_, "co:v:");
		WHEN("command line is parsed") {
			THEN("opt_map has 0 elements") {
				REQUIRE(mapSize_test(cmdlnOpts) == 0);
			}
		}
	}

	GIVEN("command line with wrong options") {
		// create argc from argv
        const char* argv_[] = { "progname", "a", "--c", "--o", "filename", "v" };
		int argc_ = (sizeof(argv_)/sizeof(argv_[0]));		
		ProgramOptions cmdlnOpts(argc_, argv_, "co:v:");
		WHEN("command line is parsed")
		{
			THEN("no arguments should inserted into opt_map") {
				REQUIRE(cmdlnOpts.exists('a') == false);
				REQUIRE(cmdlnOpts.exists('c') == false);
				REQUIRE(cmdlnOpts.exists('o') == false);
				REQUIRE(cmdlnOpts.exists('v') == false);
			}
			THEN("opt_map has 0 elements") {
				REQUIRE(mapSize_test(cmdlnOpts) == 0);
			}
		}	
	}

	GIVEN("command line with extended option and no optional arguments") {
		// create argc from argv
        const char* argv_[] = { "progname", "-o", "-c", "-v" };
		int argc_ = (sizeof(argv_)/sizeof(argv_[0]));		
		ProgramOptions cmdlnOpts(argc_, argv_, "co:v:");
		WHEN("command line is parsed")
		{
			THEN("'-c' is a simple option") {
				REQUIRE(cmdlnOpts.exists('c') == true);
				REQUIRE(cmdlnOpts.getOptArg('c') == "");
			}
			THEN("extended option '-o' does not have an 'opt arg'") {
				// "-c" is not an opt arg for "-o"
				REQUIRE(cmdlnOpts.getOptArg('o') == "");
			}
			THEN("extended option -v' at the end does not have an 'opt arg'") {
				// "-v" does not have an opt arg
				REQUIRE(cmdlnOpts.getOptArg('v') == "");
			}
			THEN("opt_map has 3 elements") {
				REQUIRE(mapSize_test(cmdlnOpts) == 3);
			}
		}	
	}

	GIVEN("command line with multiple occurances of the same option") {
		// create argc from argv
        const char* argv_[] = { "progname", "-v", "firstOptArg", "-v", "secondOptArg",
			"-o", "-c" };
		int argc_ = (sizeof(argv_)/sizeof(argv_[0]));		
		ProgramOptions cmdlnOpts(argc_, argv_, "co:v:");
		WHEN("command line is parsed")
		{
			THEN("only the first option '-v' is inserted into map, others are skipped") {
				REQUIRE(cmdlnOpts.getOptArg('v') == "firstOptArg");
			}
			THEN("extended option '-o' does not have an 'opt arg'") {
				REQUIRE(cmdlnOpts.getOptArg('o') == "");
			}
			THEN("'-c' is a simple option") {
				REQUIRE(cmdlnOpts.exists('c') == true);
				REQUIRE(cmdlnOpts.getOptArg('c') == "");
			}
			THEN("opt_map has 3 elements") {
				REQUIRE(mapSize_test(cmdlnOpts) == 3);
			}
		}	
	}
} // end SCENARIO("general option parsing", "[ProgramOptions]")

SCENARIO("video specific option parsing", "[ProgramOptions]") {
	GIVEN("command line with 4 opt args for extended options") {
		// arguments
		//  i(nput): cam (single digit number) or file name,
		//		if empty take standard cam
		//  r(ate):	fps for video device
		//  v(ideo size): frame size in px (width x height)
		//  w(orking directory): working dir, starting in $home
        const char* optstr = "i:r:v:w:";
        const char* iOptArg = "videofile.mp4";
        const char* vOptArg = "320x240";
        const char* wOptArg = "workdir";
		
		// TODO add addtional test cases:
		//  working dir with /, without /
		WHEN("all opt args are passed") {
            const char* av[] = {"progname", "-i", iOptArg, "-v", vOptArg, "-w", wOptArg};
			int ac = (sizeof(av)/sizeof(av[0]));	
			ProgramOptions po(ac, av, optstr);

			THEN("they are returnd via getOptArg()") {
				REQUIRE(po.exists('i') == true);
				REQUIRE(po.getOptArg('i') == string(iOptArg));
			
				REQUIRE(po.exists('v') == true);
				REQUIRE(po.getOptArg('v') == string(vOptArg));

				REQUIRE(po.exists('w') == true);
				REQUIRE(po.getOptArg('w') == string(wOptArg));
			}
		}

		WHEN("input option is empty") {
            const char* av[] = {"progname", "-i"};
			int ac = (sizeof(av)/sizeof(av[0]));	
			ProgramOptions po(ac, av, optstr);

			THEN("getOptArg() returns empty string and other opts don't exist") {
				REQUIRE(po.exists('i') == true);
				REQUIRE(po.getOptArg('i') == "");
			
				REQUIRE(po.exists('v') == false);
				REQUIRE(po.exists('w') == false);
			}
		}

		WHEN("same option is used multiple times") {
            const char* av[] = {"progname", "-i", iOptArg, "-i", vOptArg};
			int ac = (sizeof(av)/sizeof(av[0]));	
			ProgramOptions po(ac, av, optstr);

			THEN("first (leftmost) option is taken, the others are ignored") {
				REQUIRE(po.exists('i') == true);
				REQUIRE(po.getOptArg('i') == string(iOptArg));
				
				// others don't exist
				REQUIRE(po.exists('v') == false);
				REQUIRE(po.exists('w') == false);
			}
		}
	} // end GIVEN("command line with 4 opt args for extended options") 
} // end SCENARIO("video specific option parsing", "[ProgramOptions]") 
