#include "stdafx.h"
#pragma warning(disable: 4482) // MSVC10: enum nonstd extension

const cv::Scalar black	= cv::Scalar(0,0,0);
const cv::Scalar blue	= cv::Scalar(255,0,0);
const cv::Scalar red	= cv::Scalar(0,0,255);
const cv::Scalar gray	= cv::Scalar(128,128,128);
const cv::Scalar green	= cv::Scalar(0,255,0);
const cv::Scalar orange = cv::Scalar(0,128,255);
const cv::Scalar yellow = cv::Scalar(0,255,255);
const cv::Scalar white	= cv::Scalar(255,255,255);

// working directory for reading / writing image files
std::string g_workPath("D:\\Users\\Holger\\counter\\Skip Truck 440 - 455\\");

/// create large mask from blurred foreground mask by combining larger blobs and skipping small blobs
cv::Mat createLargeMask(cv::Mat& foregroundMask, int kernelSize, int minBlobArea);

/// calculate bounding box of small foreground mask 
cv::Rect bboxFromHighResForeGroundMask(cv::Mat foregroundMask, cv::Mat largeMask);

/// apply large mask to frame, calculate bounding box for small foreground mask, save result as image file
int processImage(long long frameNumber);

int main(int argc, char* argv[])
{

	processImage(442);

/*	for (long long i = 441; i <= 455; ++i) {
		if (processImage(i) < 0)
			break;
	}
	*/

	cv::waitKey(0);
	return 0;
}


int processImage(long long frameNumber) {
	using namespace std;

	// ------------------------------------------------------------------------
	// create large rectangular mask
	string srcPath("D:\\Users\\Holger\\counter\\Skip Truck 440 - 455\\vibe 200x200 blur 7\\");
	string imagePath = srcPath + "mask_" + std::to_string(frameNumber) + ".png";
	cv::Mat src = cv::imread(imagePath);
	if (!src.data) {
		cout << "cannot read image path: " << imagePath << endl;
		cout << endl << "Press <enter> to exit" << endl;
		string str;
		getline(cin, str);
		return -1;
	}
	
	// must be one channel
	cv::Mat srcGrey;
	cv::cvtColor(src, srcGrey, cv::COLOR_BGR2GRAY);

	cv::Mat maskLarge = createLargeMask(srcGrey, 21, 1000);
	
	// ------------------------------------------------------------------------
	// create high res mask and bounding rect
	srcPath = "D:\\Users\\Holger\\counter\\Skip Truck 440 - 455\\vibe 200x200 blur 3\\";
	imagePath = srcPath + "mask_" + std::to_string(frameNumber) + ".png";
	src = cv::imread(imagePath);
	if (!src.data) {
		cout << "cannot read image path: " << imagePath << endl;
		cout << endl << "Press <enter> to exit" << endl;
		string str;
		getline(cin, str);
		return -1;
	}
	// must be one channel
	cv::Mat foregroundMask;
	cv::cvtColor(src, foregroundMask, cv::COLOR_BGR2GRAY);

	cv::Rect bbox = bboxFromHighResForeGroundMask(foregroundMask, maskLarge);

	// ------------------------------------------------------------------------
	// read and mask frame
	srcPath = "D:\\Users\\Holger\\counter\\Skip Truck 440 - 455\\frame 200x200\\";
	imagePath = srcPath + "frame_" + std::to_string(frameNumber) + ".png";

	cv::Mat frame = cv::imread(imagePath);
	if (!frame.data) {
		cout << "cannot read image path: " << imagePath << endl;

		cout << endl << "Press <enter> to exit" << endl;
		string str;
		getline(cin, str);

		return -1;
	}

	cv::Mat maskedFrame;
	frame.copyTo(maskedFrame, maskLarge);

	// overlay bbox
	cv::rectangle(maskedFrame, bbox, green);

	// show images
	cv::imshow("src", srcGrey);
	cv::imshow("masked frame", maskedFrame);
	
	// write dilated result to file
	string dstPath("D:\\Users\\Holger\\counter\\Skip Truck 440 - 455\\vibe 200x200 dilate\\blur 7\\");
	imagePath = dstPath + "2_frame_masked_" + std::to_string(frameNumber) + ".png";

	cv::imwrite(imagePath, maskedFrame);

	return 0;
}


cv::Mat createLargeMask(cv::Mat& foregroundMask, int kernelSize, int minBlobArea) {
	cv::Mat rectMask;
	// error, if image not binary
	if (foregroundMask.channels() > 1) {
		std::cerr << "createLargeMask: too many color channels, foregroundMask must be grayscale" << std::endl;
		return rectMask;
	// create rectangular mask
	} else { 
		rectMask = cv::Mat(foregroundMask.size(), CV_8UC1, cv::Scalar(0));
	}

	// apply morph function 21x21
	cv::Mat morphResult;
	cv::Mat kernel = cv::getStructuringElement(cv::MorphShapes::MORPH_RECT, cv::Size(kernelSize,kernelSize));
	cv::dilate(foregroundMask, morphResult, kernel, cv::Point(-1,-1), 1);

	// find contours of foreground mask
	std::vector<std::vector<cv::Point> > contours;
	std::vector<cv::Vec4i> hierarchy;
	cv::findContours(morphResult, contours, hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, cv::Point(0,0));

	// calculate bounding boxes of all contours
	// collect bbox corner points, if bbox area is large
	std::vector<cv::Point> maskPoints;
	for (unsigned int i = 0; i < contours.size(); ++i) { 
		cv::Rect bbox = cv::boundingRect(contours[i]);
		// DEBUG std::cout << i << " bbox original : " << bbox << std::endl;

		if (bbox.area() > minBlobArea) {
				// push back all corner points
				cv::Point topLeft(bbox.tl());
				cv::Point topRight(bbox.tl() + cv::Point(bbox.width,0));
				cv::Point bottomLeft(bbox.tl() + cv::Point(0, bbox.height));
				cv::Point bottomRight(bbox.br());
				maskPoints.push_back(topLeft);
				maskPoints.push_back(topRight);
				maskPoints.push_back(bottomLeft);
				maskPoints.push_back(bottomRight);
		}
	}

	cv::Rect rect = cv::boundingRect(maskPoints);
	cv::rectangle(rectMask, rect, white, CV_FILLED);

	// DEBUG ------------------------------------------------------------------
	// prepare dst image for contour printing
	cv::Mat dstColor(rectMask.size(), CV_8UC3);
	cv::cvtColor(foregroundMask, dstColor, cv::COLOR_GRAY2BGR);
	
	// draw all contours
	for (unsigned int i = 0; i < hierarchy.size(); ++i) 
		cv::drawContours(dstColor, contours, i, green);
	/*
	for (unsigned int j = 0; j < bboxes.size(); ++j) {
		cout << "rect " << j << ": " << bboxes.at(j) << endl;
	}
	cout << endl;

	// draw hulls
	for (unsigned int i = 0; i < hulls.size(); ++i) 
		cv::drawContours(dstColor, hulls, i, red);

	// draw bboxes
	for (unsigned int i = 0; i < bboxes.size(); ++i) 
		cv::rectangle(dstColor, bboxes.at(i), yellow);
	*/

	// draw enclosing bbox
	cv::Rect maskRect = cv::boundingRect(maskPoints);
	cv::rectangle(dstColor, maskRect, blue, 2);

	cv::imshow("contours" , dstColor);
	
	// END_DEBUG --------------------------------------------------------------

	return rectMask;
}



cv::Rect bboxFromHighResForeGroundMask(cv::Mat foregroundMask, cv::Mat largeMask) {
	cv::Mat largeMaskForeground;
	foregroundMask.copyTo(largeMaskForeground, largeMask);

	std::vector<std::vector<cv::Point> > contours;
	cv::findContours(largeMaskForeground, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, cv::Point(0,0));
	int x_max = -1;
	int y_max = -1;
	int x_min = foregroundMask.size().width + 1;
	int y_min = foregroundMask.size().height + 1;
	for (unsigned int n = 0; n < contours.size(); ++n) {
		for (unsigned int point = 0; point < contours.at(n).size(); ++point) {
			contours.at(n).at(point).x < x_min ? x_min = contours.at(n).at(point).x : x_min;
			contours.at(n).at(point).y < y_min ? y_min = contours.at(n).at(point).y : y_min;
			contours.at(n).at(point).x > x_max ? x_max = contours.at(n).at(point).x : x_max;
			contours.at(n).at(point).y > y_max ? y_max = contours.at(n).at(point).y : y_max;
		}
	}

	cv::Rect bbox(x_min, y_min, x_max - x_min, y_max - y_min);

	// DEBUG ------------------------------------------------------------------
	// prepare dst image for contour printing
	cv::Mat dstColor(foregroundMask.size(), CV_8UC3);
	cv::cvtColor(foregroundMask, dstColor, cv::COLOR_GRAY2BGR);
	for (unsigned int i = 0; i < contours.size(); ++i) 
	// for (unsigned int i = 0; i < 1; ++i) 
		cv::drawContours(dstColor, contours, i, green);
	cv::rectangle(dstColor, bbox, blue);
	cv::imshow("high res", dstColor);
	// END_DEBUG --------------------------------------------------------------


	return bbox;
}


