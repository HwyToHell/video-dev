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

/// create large mask from blurred foreground mask by dilating (input: kernel size)
/// and combining larger blobs (input: minimum area)
cv::Mat createLargeMask(cv::Mat& foregroundMask, int kernelSize, int minBlobArea, cv::Mat* debugImage);

/// calculate bounding box of small foreground mask 
cv::Rect bboxFromHighResForeGroundMask(cv::Mat foregroundMask, cv::Mat largeMask);

/// apply large mask to frame, calculate bounding box for small foreground mask, save result as image file
int processImage(long long frameNumber);

void joinRectX_test();

int main_post_process(int argc, char* argv[])
{
	using namespace std;

	//processImage(62);
	//joinRectX_test();


	for (long long i = 50; i <= 67; ++i) {
		if (processImage(i) < 0)
			break;
	}


	cv::waitKey(0);


	
	cout << endl << "Press <enter> to exit" << endl;
	string str;
	getline(cin, str);
	
	return 0;
}


int processImage(long long frameNumber) {
	using namespace std;

	// ------------------------------------------------------------------------
	// create large rectangular mask (low res from medianBlur 7x7)
	string srcPath("D:\\Users\\Holger\\counter\\50 - 67 two cars opposite\\blur 7\\");
	string imagePath = srcPath + "mask_" + std::to_string(frameNumber) + ".png";
	//string imagePath = srcPath + "5x4" + ".png";
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

	cv::Mat debugImage(src.size(), CV_8UC3, black);
	cv::Mat maskLarge = createLargeMask(srcGrey, 21, 600, &debugImage);
	cv::imshow("debug", debugImage);

	// write post-processed result to file
	string dstPath("D:\\Users\\Holger\\counter\\054 - 067 two cars opposite\\");
	imagePath = dstPath + "large_mask_" + std::to_string(frameNumber) + ".png";
	cv::imwrite(imagePath, maskLarge);
	
	/*
	// ------------------------------------------------------------------------
	// create high res mask and bounding rect
	srcPath = "D:\\Users\\Holger\\counter\\dilate\\";
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
	*/

	/*
	// ------------------------------------------------------------------------
	// read and mask frame
	srcPath = "D:\\Users\\Holger\\counter\\054 - 067 two cars opposite\\frame 200x200\\";
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
	
	// write post-processed result to file
	string dstPath("D:\\Users\\Holger\\counter\\054 - 067 two cars opposite\\");
	imagePath = dstPath + "frame_masked_" + std::to_string(frameNumber) + ".png";
	cv::imwrite(imagePath, maskedFrame);
	*/

	return 0;
}


bool isYdistZeroOrLess(cv::Rect rc1, cv::Rect rc2) {
	cv::Rect rcTop, rcBottom;
	if (rc1.y < rc2.y) {
		rcTop = rc1;
		rcBottom = rc2;
	} else {
		rcTop = rc2;
		rcBottom = rc1;
	}

	if (rcBottom.y - (rcTop.y + rcTop.height) <= 0)
		return true;
	else 
		return false;
}


cv::Rect joinRectX(std::vector<cv::Rect>& exist, cv::Rect rcNew, int maxDistXJoin) {
	for (int i = 0; i < exist.size(); ++i) {

		// assign left and right rectangle for proper subtraction
		cv::Rect rcLeft, rcRight;
		if (exist[i].x < rcNew.x) {
			rcLeft = exist[i];
			rcRight = rcNew;
		} else {
			rcLeft = rcNew;
			rcRight = exist[i];
		}

		// join with matching rect, depending on x distance
		if (rcRight.x - (rcLeft.x + rcLeft.width) < maxDistXJoin) {
			if (isYdistZeroOrLess(rcLeft, rcRight)) {
				exist[i] = rcLeft | rcRight;
				return exist[i];
			}
		} 
	}

	// no match found, append new rect
	exist.push_back(rcNew);		
	return rcNew;
}


void joinRectX_test() {
	using namespace std;
	vector<cv::Rect> exist;
	int maxDist = 10;
	exist.push_back(cv::Rect(100, 0, 50, 10));
	exist.push_back(cv::Rect(0, 0, 50, 10));

	// complete vector before joining
	cout << "exist rect vector before joining" << endl;
	for (int i = 0; i < exist.size(); ++i) {
		cout << i << " rect: " << exist[i] << endl;
	}
	cout << endl;


	// x distance 5, right from second rect --> join
	cv::Rect ret = joinRectX(exist, cv::Rect(55, 0, 15, 20), 10);
	cout << "joined rect: " << ret << endl << endl;

	// x distance 5, left from first rect --> join
	ret = joinRectX(exist, cv::Rect(90, 0, 5, 30), 10);
	cout << "joined rect: " << ret << endl << endl;

	// x distance 50 --> don't join
	ret = joinRectX(exist, cv::Rect(200, 0, 50, 22), 10);
	cout << "appended rect: " << ret << endl << endl;

	// x distance < 0, y distance > 0 --> don't join
	ret = joinRectX(exist, cv::Rect(100, 50, 33, 55), 10);
	cout << "appended rect: " << ret << endl << endl;


	// complete vector after joining
	cout << "exist rect vector after joining" << endl;
	for (int i = 0; i < exist.size(); ++i) {
		cout << i << " rect: " << exist[i] << endl;
	}
	return;
}


cv::Mat createLargeMask(cv::Mat& foregroundMask, int kernelSize, int minBlobArea, cv::Mat* debugImage) {
	cv::Mat largeMask;
	// error, if image not binary
	if (foregroundMask.channels() > 1) {
		std::cerr << "createLargeMask: too many color channels, foregroundMask must be grayscale" << std::endl;
		return largeMask;
	// create rectangular mask
	} else { 
		largeMask = cv::Mat(foregroundMask.size(), CV_8UC1, cv::Scalar(0));
	}

	// apply morph function 21x21
	cv::Mat morphResult;
	cv::Mat kernel = cv::getStructuringElement(cv::MorphShapes::MORPH_RECT, cv::Size(kernelSize,kernelSize));
	cv::dilate(foregroundMask, morphResult, kernel, cv::Point(-1,-1), 1);

	// find contours of foreground mask
	std::vector<std::vector<cv::Point> > contours;
	std::vector<cv::Rect> boundingBoxes;
	cv::findContours(morphResult, contours, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, cv::Point(0,0));

	// calculate bounding boxes of all contours
	// collect bbox corner points, if bbox area is large
	for (unsigned int i = 0; i < contours.size(); ++i) { 
		cv::Rect bbox = cv::boundingRect(contours[i]);
		// DEBUG 
		//std::cout << i << " bbox size : " << bbox << std::endl;
		//std::cout << i << " bbox area : " << bbox.area() << std::endl << std::endl;
		
		if (bbox.area() > minBlobArea) {
			// check proximity
			joinRectX(boundingBoxes, bbox, 10);
		}

	}

	// create large mask from bounding boxes of motion objects
	for (unsigned int i = 0; i < boundingBoxes.size(); ++i) { 
		cv::rectangle(largeMask, boundingBoxes[i], white, CV_FILLED);
	}


	// DEBUG ------------------------------------------------------------------
	// prepare dst image for contour printing
	if (debugImage) {
		foregroundMask.copyTo(*debugImage);
		cv::cvtColor(*debugImage, *debugImage, cv::COLOR_GRAY2BGR);	
		// check size and color format
		/*CV_Assert(debugImage->size() == foregroundMask.size());
		if (debugImage->type() == CV_8UC1)
			cv::cvtColor(*debugImage, *debugImage, cv::COLOR_GRAY2BGR);	
		CV_Assert(debugImage->type() == CV_8UC3);
		*/
	
		// draw all contours
		for (unsigned int i = 0; i < contours.size(); ++i) 
			cv::drawContours(*debugImage, contours, i, green);
		
		// draw bounding rect for all contours
		for (unsigned int i = 0; i < contours.size(); ++i) {
			cv::Rect bbox = cv::boundingRect(contours[i]);
			cv::rectangle(*debugImage, bbox, yellow);
		}



		/*
		for (unsigned int j = 0; j < bboxes.size(); ++j) {
			cout << "rect " << j << ": " << bboxes.at(j) << endl;
		}
		cout << endl;
		
		// draw hulls
		for (unsigned int i = 0; i < hulls.size(); ++i) 
			cv::drawContours(debugImage, hulls, i, red);

		// draw bboxes
		for (unsigned int i = 0; i < bboxes.size(); ++i) 
			cv::rectangle(debugImage, bboxes.at(i), yellow);
		*/

		// draw bbox of independent objects
		for (unsigned int i = 0; i < boundingBoxes.size(); ++i) {
			cv::rectangle(*debugImage, boundingBoxes[i], blue, 2);
		}

	}
	// END_DEBUG --------------------------------------------------------------

	return largeMask;
}


// TODO adapt to work with roi instead of mask
// benefit: multiple bboxes cannot be calculated from mask
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
	/*
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
	*/

	return bbox;
}


