#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include <iostream>
#include <string>

#include <opencv2/opencv.hpp>
#include "../../app-dev/cpp/inc/pick_list.h"
#include "../../app-dev/video-dev/car-count/include/tracker.h"

Track createTrackAt(const cv::Size roi, const cv::Point blobPos, const cv::Size blobSize, const cv::Point velocity, const size_t id);
Occlusion createOcclusionAt(Track& trackRight, Track& trackLeft,
	const cv::Size roi,  const int collisionX, const cv::Size blobSize, const cv::Point velocityRight, const cv::Point velocityLeft);

bool isVisualTrace;

int main(int argc, char* argv[]){
	using namespace std;

	//string cases("[Track],[Scene],[Occlusion]");
	string cases("[Scene]");
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
	isVisualTrace = true;

	int result = Catch::Session().run(ac, av);
	//int result = Catch::Session().run(argc, argv);

	
	cout << "Press <enter> to continue" << endl;
	string str;
	getline(cin, str);
	return 0;
}

