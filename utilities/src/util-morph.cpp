#include "stdafx.h"
#include "../../../video-dev/car-count/include/frame_handler.h"

#pragma warning(disable: 4482) // MSVC10: enum nonstd extension
using namespace std;

int morph_main(int argc, char* argv[]) {
	string path = "D:\\Users\\Holger\\counter\\MOG-org\\";
	string filePrefix = "mask_42";
	string fileExt = ".png";
	string filePath = path + filePrefix + fileExt;	
	cv::Mat mask = cv::imread(filePath, cv::IMREAD_GRAYSCALE);

	// morphology
	cv::Mat morph;
	//cv::Mat kernel = cv::getStructuringElement(cv::MorphShapes::MORPH_ELLIPSE, cv::Size(11,11));
	//cv::morphologyEx(mask, morph, cv::MorphTypes::MORPH_CLOSE, kernel);
	cv::dilate(mask, morph, cv::Mat(11,11,CV_8UC1));
	cv::erode(morph, morph, cv::Mat(13,13,CV_8UC1));

	// find contours
	std::vector<std::vector<cv::Point> > contours;
	std::vector<cv::Vec4i> hierarchy;
	cv::findContours(morph, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, cv::Point(0,0));

	cv::Mat morphColor;
	cv::cvtColor(morph, morphColor, cv::COLOR_GRAY2BGR);

	// convex hull
	std::vector<std::vector<cv::Point> > hulls(contours.size());
	std::vector<cv::Rect> rects(hulls.size());
	for (int i = 0; i < hierarchy.size(); ++i) {
		cv::convexHull(contours[i], hulls[i]);
		rects[i] = cv::boundingRect(hulls[i]);

		cv::drawContours(morphColor, contours, i, red);
		cv::drawContours(morphColor, hulls, i, green);
		cv::rectangle(morphColor, rects[i], blue);
	}


//	for (int i = 0; i < hierarchy.size(); ++i)
//		cv::drawContours(morphColor, contours, i, red);





	cv::imshow("mask", mask);
	cv::imshow("morph", morphColor);

	cv::waitKey(0);
	/*
	cout << endl << "Press <enter> to exit" << endl;
	string str;
	getline(cin, str);
	*/
	return 0;
}