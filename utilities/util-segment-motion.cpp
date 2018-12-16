#include "stdafx.h"
#include <climits>
#pragma warning(disable: 4482) // MSVC10: enum nonstd extension

#include "D:/Holger/app-dev/video-dev/car-count/include/config.h"
#include "D:/Holger/app-dev/video-dev/car-count/include/frame_handler.h"
#include "D:/Holger/app-dev/video-dev/car-count/include/tracker.h"
#include "D:/Holger/app-dev/video-dev/car-count/include/recorder.h"
//#include "../../inc/program_options.h"
#include "D:/Holger/app-dev/cpp/inc/opencv/backgroundsubtraction.h"

using namespace std;

cv::Mat getCrossKernel(int kernelSize, double percent);
cv::Mat getCrossKernel(int kernelSize, int crossArmSize);



class Segmentation {
public:
	Segmentation(int blobAreaMin, int blobAreaMax);
	std::vector<cv::Rect>	getBBoxes(cv::Mat& mask);
	double					getThreshold();
	cv::Mat					segmentFrame(cv::Mat& frame);
	void					showBboxes(cv::Mat& frame);
	void					showContours(cv::Mat& frame);
	void					showHulls(cv::Mat& frame);
private:
	cv::Ptr<cv::BackgroundSubtractorMOG2>	m_mog2;
	int										m_blobAreaMin;
	int										m_blobAreaMax;
	std::vector<cv::Rect>					m_bboxes;
	std::vector<std::vector<cv::Point> >	m_contours;
	std::vector<cv::Vec4i>					m_hierarchy;
	std::vector<std::vector<cv::Point> >	m_hulls;
};






boost::mutex mtxImage;
boost::mutex mtxLabel;

boost::condition_variable cndImage;
bool isImageAvailable;
bool isSourceFeeding;
cv::Mat srcImage;

int label;


// classification thread: wait for new image, wake thread, lock srcImage when reading
void waitForNewImage(cv::Mat& srcImage, cv::Mat& dstImage) {
	boost::unique_lock<boost::mutex> lock(mtxImage);
	while(!isImageAvailable) {
		cndImage.wait(lock);
	}
	dstImage = srcImage.clone();
	isImageAvailable = false;
	cndImage.notify_one();
	return;
}

// main thread: wait for image buffer to be available
void waitForImageBuffer(cv::Mat& image, cv::Rect roi, cv::Mat& imgBuf) {
	boost::unique_lock<boost::mutex> lock(mtxImage);
	while(isImageAvailable) {
		cndImage.wait(lock);
	}
	imgBuf = image(roi);
	isImageAvailable = true;
	cndImage.notify_one();
	return;
}

// write classification results to other thread
void writeClassification(int classLabel) {
	mtxLabel.lock();
	classLabel = 1;
	mtxLabel.unlock();
	return;
}


// thread
void classify(cv::Mat& srcImage) {
	boost::chrono::seconds duration(1);
	cv::Mat blob;

	while(isSourceFeeding) {

		waitForNewImage(srcImage, blob);

		for (int i = 1; i < 10; ++i) {
			using namespace boost::this_thread;
			sleep_for(duration);
			cout << "classifying for " << i << " sec" << endl;
		}
		cout << "finished" << endl << endl;

		// writeClassification(label);
	}
}

/// create large mask from blurred foreground mask by dilating (input: kernel size)
/// and combining larger blobs (input: minimum area)
cv::Mat createLargeMask(cv::Mat& foregroundMask, int kernelSize, int minBlobArea, cv::Mat* debugImage);

/// calculate bounding box of small foreground mask 
cv::Rect bboxFromHighResForeGroundMask(cv::Mat foregroundMask, cv::Mat largeMask);


/// background subtraction of input video, store segmentation results frame by frame to out path 
int segmentMotion(
		int startFrame,
		int stopFrame,
		cv::Rect roi, 
		std::string videoPath = "D:\\Users\\Holger\\counter\\traffic640x480.avi",
		std::string outPath = "D:\\Users\\Holger\\counter\\segment-motion\\");

int main_segment(int argc, char* argv[]) {

	// cv::Rect roi(250, 200, 200, 200); // roi 640x480
	cv::Rect roi(120, 70, 100, 100);	 // roi 320x240
	string path320 = "D:\\Users\\Holger\\counter\\traffic320x240.avi";

	// 2, 20	// opposite two cars
	// 105, 116 // single car to right
	// 244, 260 // opposite car + car with trailer
	// 653, 715 // truck, car, bus
	segmentMotion(653, 715, roi, path320);

	cout << endl << "Press <enter> to exit" << endl;
	string str;
	getline(cin, str);
	
	return 0;
}

int segmentMotion(int startFrame, int stopFrame, cv::Rect roi, std::string videoPath, std::string outPath) {
	using namespace std;


	// validate video path
	if (!isFileExist(videoPath)) {
		cerr << "video path not valid: " << videoPath << endl;
		return -1;
	}

	// capture video from file
	cv::VideoCapture cap;
	cap.open(videoPath);
	cv::Mat image;

	// status variables	
	long long frameCount = 0;
	
	// TODO delete // try different background subtractors
	BackgroundSubtractorLowPass lowPass(0.005, 40);
	BackgroundSubtractorVibe vibe;

	while (cap.read(image)) {

		++frameCount;
		cv::rectangle(image, roi, green);
		cv::putText(image, to_string(frameCount), cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1, green, 2);
		cv::imshow("frame", image);


		// apply roi (200x200) and different background subtraction algorithms 
		cv::Mat frame_roi = image(roi);
		cv::imshow("frame 200x200", frame_roi);

		/*
		// vibe rgb
		// original size 200x200
		cv::Mat vibemask;
		cv::Mat frame_roi_smooth;
		cv::blur(frame_roi, frame_roi_smooth, cv::Size(5,5));
		vibe.apply(frame_roi_smooth, vibemask);
		cv::imshow("vibe 200x200", vibemask);
		*/

		// low pass
		cv::Mat lpmask;
		cv::Mat frame_roi_smooth;
		//cv::blur(frame_roi, frame_roi_smooth, cv::Size(5,5));
		lowPass.apply(frame_roi, lpmask);
		cv::imshow("low pass", lpmask);

		// post-processing
		// create large mask
		cv::Mat highResMask, lowResMask;
		cv::medianBlur(lpmask, lowResMask, 5);
		//int dilateKernel = 21; // 640x480
		//int minBlobArea = 702; // 640x480
		int dilateKernel = 11;
		int minBlobArea = 196;

		cv::Mat debugImg(frame_roi.size(), CV_8UC3, black);
		cv::Mat largeMask = createLargeMask(lowResMask, dilateKernel, minBlobArea, &debugImg);
		cv::imshow("debug", debugImg);

		/*
		// bbox with large masked high res segmentation result
		cv::medianBlur(vibemask, highResMask, 3);
		cv::Rect bbox = bboxFromHighResForeGroundMask(highResMask, largeMask);
		*/


		// create masked frame in color
		cv::Mat maskedFrame;
		frame_roi.copyTo(maskedFrame, largeMask);
		cv::putText(maskedFrame, to_string(frameCount), cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 1, green, 2);
		cv::imshow("moving objects", maskedFrame);
		
		// DEBUG frame sequence of interest
		// save segmentation results to image files
		//if (false) {
		if (frameCount >= startFrame && frameCount <= stopFrame) {
			/*
			// blur 7
			string path = "D:\\Users\\Holger\\counter\\440 - 455 skip truck\\blur 7\\";
			string filePath = path + "mask_" + to_string(frameCount) + ".png";
			cv::imwrite(filePath, lowResMask);

			// blur 3
			path = "D:\\Users\\Holger\\counter\\440 - 455 skip truck\\blur 3\\";
			filePath = path + "mask_" + to_string(frameCount) + ".png";
			cv::imwrite(filePath, highResMask);

			// blur 7 + dilate 21
			cv::Mat morphResult;
			int kernelSize = 21;
			cv::Mat kernel = cv::getStructuringElement(cv::MorphShapes::MORPH_RECT, cv::Size(kernelSize,kernelSize));
			cv::dilate(lowResMask, morphResult, kernel, cv::Point(-1,-1), 1);
			path = "D:\\Users\\Holger\\counter\\440 - 455 skip truck\\dilate 21\\";
			filePath = path + "mask_" + to_string(frameCount) + ".png";
			cv::imwrite(filePath, morphResult);
			*/
			cout << "frame: " << frameCount << endl;
			// debug image
			string path = "D:\\Users\\Holger\\counter\\segment-motion\\";
			string filePath = path + "debug_" + to_string(frameCount) + ".png";
			cv::imwrite(filePath, debugImg);

			//filePath = path + "frame_" + to_string(frameCount) + ".png";
			//cv::imwrite(filePath, frame_roi);

			//filePath = path + "lowpass_" + to_string(frameCount) + ".png";
			//cv::imwrite(filePath, lpmask);

			//filePath = path + "lowResMask_" + to_string(frameCount) + ".png";
			//cv::imwrite(filePath, lowResMask);

			
			//filePath = path + "vibe_" + to_string(frameCount) + ".png";
			//cv::imwrite(filePath, vibemask);
		}

		if (frameCount > stopFrame)
			return -1;

		// break loop with ESC
		if (cv::waitKey(10) == 27) 	{
			cout << "ESC pressed -> end video processing" << endl;
			//cv::imwrite("frame.jpg", frame);
			break;
		}
	}
	return 0;
}


Segmentation::Segmentation(int blobAreaMin, int blobAreaMax) :
	m_blobAreaMin(blobAreaMin),
	m_blobAreaMax(blobAreaMax) {
	m_mog2 = cv::createBackgroundSubtractorMOG2(100, 25, false);
}


std::vector<cv::Rect> Segmentation::getBBoxes(cv::Mat& mask) {
	std::vector<cv::Rect> bboxes;

	// find contours
	std::vector<std::vector<cv::Point> > contours;
	std::vector<cv::Vec4i> hierarchy;
	cv::findContours(mask, m_contours, m_hierarchy, CV_RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, cv::Point(0,0));
	
	
	// DEBUG
	//cout << "contours: " << m_contours.size() << endl;
	//cout << "hierarchy: " << m_hierarchy.size() << endl << endl;

	// calc bounding boxes, check limits
	// calc bounding boxes of all detections and store in m_bBoxes
	m_bboxes.clear();
	m_hulls.clear();
	for (unsigned int i = 0; i < m_contours.size(); i++) { 
		// convex hull of contour
		std::vector<cv::Point> hull;
		cv::convexHull(m_contours[i], hull);
		cv::Rect bbox = cv::boundingRect(hull);
		// DEBUG
		// cout << "  bbox: " << bbox << endl;
		if ((bbox.area() > m_blobAreaMin) && (bbox.area() < m_blobAreaMax)) {
				m_bboxes.push_back(bbox);
				m_hulls.push_back(hull);
		}
	}

	return bboxes;
}

double Segmentation::getThreshold() {
	return m_mog2->getVarThreshold();
}


cv::Mat Segmentation::segmentFrame(cv::Mat& frame) {
	//cv::blur(frame, frame, cv::Size(12, 12));
	cv::Mat mask;
	//cv::medianBlur(frame, frame, 7);
	m_mog2->apply(frame, mask);
	//cv::medianBlur(mask, mask, 3);
	//cv::Mat kernel = cv::getStructuringElement(cv::MorphShapes::MORPH_CROSS, cv::Size(11,11));
	//cv::Mat kernel = getCrossKernel(13, 3);
	// DEBUG 
	// cout << "kernel: " << endl << kernel << endl;

	//cv::morphologyEx(mask, mask, cv::MorphTypes::MORPH_DILATE, kernel);
	//cv::dilate(mask, mask, cv::Mat(11,11,CV_8UC1));
	//cv::erode(mask, mask, cv::Mat(15,15,CV_8UC1));
	return mask;
	//return frame;
}

void Segmentation::showBboxes(cv::Mat& frame) {
    for (int i = 0; i < m_bboxes.size(); ++i) {
		cv::rectangle(frame, m_bboxes.at(i), green);
	}
}

void Segmentation::showContours(cv::Mat& frame) {
	for (int i = 0; i < m_hierarchy.size(); ++i) 
		cv::drawContours(frame, m_contours, i, red);
	return;
}


void Segmentation::showHulls(cv::Mat& frame) {
	for (int i = 0; i < m_hulls.size(); ++i) 
		cv::drawContours(frame, m_hulls, i, yellow);
	return;
}


cv::Mat getCrossKernel(int kernelSize, int crossArmSize) {
	cv::Mat kernel(kernelSize, kernelSize, CV_8UC1);

	if (crossArmSize < 1) {
		crossArmSize = 1;
		cout << "crossArmSize set to 1, must be larger than 0" << endl;
	}

	if (crossArmSize > kernelSize) {
		crossArmSize = kernelSize;
		cout << "crossArmSize set to kernelSize, must be smaller or equal to kernelSize" << endl;
	}

	int colSetStart = (kernelSize - crossArmSize) / 2;
	int rowSetStart = (kernelSize - crossArmSize) / 2;
	int colSetStop = colSetStart + crossArmSize;
	int rowSetStop = rowSetStart + crossArmSize;

	int row, col;
	for (row = 0; row < kernelSize; ++row) {
		unsigned char* ptr = kernel.ptr(row);
		
		// row outside cross arm
		if (row < rowSetStart || row >= rowSetStop)
			for (col = 0; col < kernelSize; ++col) 
				// column outside cross arm
				if (col < colSetStart || col >= colSetStop) 
					ptr[col] = 0;
				// inside cross arm
				else 
					ptr[col] = 1;
		// row inside cross arm
		else
			for (col = 0; col < kernelSize; ++col) 
					ptr[col] = 1;
	}

	return kernel;
}


cv::Mat getCrossKernel(int kernelSize, double percent) {
	double crossArmWidthDouble = static_cast<double> (kernelSize * percent) / 100;
	int crossArmWidth = static_cast<int> (round (crossArmWidthDouble));

	return getCrossKernel(kernelSize, crossArmWidth);
}


