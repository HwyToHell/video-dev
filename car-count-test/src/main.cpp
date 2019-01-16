#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include <iostream>
#include <string>



int main(int argc, char* argv[]){
	using namespace std;

	string cases("[Scene]");
	const int ac = 2; // # of cmd line arguments for catch app 
	const char* av[ac];
	av[0] = argv[0];
	av[1] = cases.c_str();

	int result = Catch::Session().run(ac, av);
	//int result = Catch::Session().run(argc, argv);

	
	cout << "Press <enter> to continue" << endl;
	string str;
	getline(cin, str);
	return 0;
}

