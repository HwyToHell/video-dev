#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include <iostream>
#include <string>

#include <opencv2/opencv.hpp>
#include "../../cpp/inc/pick_list.h"
#include "../../video-dev/car-count/include/tracker.h"

Track createTrackAt(cv::Size roi, cv::Point blobPos, cv::Size blobSize, cv::Point velocity, size_t id);
Occlusion createOcclusionAt(Track& trackRight, Track& trackLeft,
	cv::Size roi, int collisionX, cv::Size blobSize, cv::Point velocityRight, cv::Point velocityLeft);

int main(int argc, char* argv[]){
	using namespace std;

	string cases("[Track],[Scene],[Occlusion]");
	//string cases("[Scene]");
	const int ac = 2; // # of cmd line arguments for catch app 
	const char* av[ac];
	av[0] = argv[0];
	av[1] = cases.c_str();

	//cout << cv::getBuildInformation();

	/*
	// test functions
	cv::Size roi(100,100);
	cv::Point velRight(6,0);
	cv::Point velLeft(-5,0);
	cv::Point blobPos(10,70);
	cv::Size blobSize(30,20);

	Track track = createTrackAt(roi, blobPos, blobSize, velLeft, 1);
	Track trackRight;
	Track trackLeft;
	Occlusion occ = createOcclusionAt(trackRight, trackLeft, roi, 50, blobSize, velRight, velLeft);
	waitForEnter();
	return 0;
	*/
	int result = Catch::Session().run(ac, av);
	//int result = Catch::Session().run(argc, argv);

	
	cout << "Press <enter> to continue" << endl;
	string str;
	getline(cin, str);
    return result;
}

