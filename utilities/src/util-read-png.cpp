#include "../../car-count/src/stdafx.h"
#include <ctime>
#include <boost/filesystem.hpp>
#include "D:/Holger/app-dev/video-dev/car-count/include/frame_handler.h"
#include "D:/Holger/app-dev/video-dev/car-count/include/tracker.h"
#include "D:/Holger/app-dev/video-dev/utilities/inc/util-visual-trace.h"

#if !defined(__GNUG__) && (_MSC_VER == 1600)
#pragma warning(disable: 4482) // MSVC10: enum nonstd extension
#endif


/// locate work path with post-processed frames based on frame number
std::string findWorkPath(int startFrame, int stopFrame, std::string descendPath);

/// read track debug frame from directory
cv::Mat readImage(std::string directory, std::string fileName, long long frameNumber);

/// set roi size in config file
bool setConfigRoiSize(Config* pConfig, std::string workPath, std::string filePrefix, long long startFrame);

/// read image sequence
bool trackImageSequence(SceneTracker* pScene, std::string directory, std::string file, long long start, long long stop, bool isManual = true);

/// extract motion rectangles with specified color from debug frame
std::vector<cv::Rect> motionRectsFromDebugFrame(cv::Mat imageBGR, cv::Scalar color,
	double deviationHue = 0.01, double deviationSatVal = 0.1);

/// show prior and current track entries in track image
void printTrackUpdate(cv::Mat trackImage, std::list<Track>* pDebugTracks);


/// return scaled image showing prior and current track entries
cv::Mat printTrackUpdateScaled(std::list<Track>* pDebugTracks, cv::Size roi, int scaling = 1);


/// create string with current time information
std::string timeNow_YMD_HMS();

/// pause console until <enter> has been pressed
bool waitForEnter();


cv::Size getConfigRoiSize(Config* pConfig) {
	cv::Size roi;
	roi.width = std::stoi(pConfig->getParam("roi_width"));
	roi.height = std::stoi(pConfig->getParam("roi_height"));
	return roi;
}


std::string findWorkPath(int startFrame, int stopFrame, std::string descendPath) {
	namespace fs = boost::filesystem;
	using namespace std;

	// directory prefix to look for
    string dirPrefix = std::to_string(static_cast<long long>(startFrame)) + " - " + std::to_string(static_cast<long long>(stopFrame));
	string workPath("");
	
	// return empty string, if path does not exist
	fs::path descendPathFS(descendPath);
	if (!fs::exists(descendPathFS)) {
		cerr << "work path does not exist: " << descendPath << endl;
		return "";
	}

	// list descendPath content
	fs::directory_iterator iDir(descendPathFS);
	while (iDir != fs::directory_iterator()) {
		//cout << iDir->path().filename() << endl;
		string dir(iDir->path().filename().string());
		if (dir.find(dirPrefix) != string::npos) {
			workPath = iDir->path().generic_path().string();
			break;
		}
		++iDir;
	}

	if (iDir != fs::directory_iterator()) {
		cout << "found path: " << workPath << endl;
		return workPath;
	} else {
		cout << "cannot find path with prefix: " << dirPrefix << endl;
		return "";
	}
}


std::vector<cv::Rect> motionRectsFromDebugFrame(cv::Mat imageBGR, cv::Scalar colorBGR, 
	double deviationHue, double deviationSatVal) {
	
	// convert BGR to HSV color space for better separation based on single color value
    cv::Vec3b colorVectorBGR(static_cast<unsigned char>(colorBGR(0)),
                             static_cast<unsigned char>(colorBGR(1)), static_cast<unsigned char>(colorBGR(2)));
	cv::Vec3b colorVectorHSV(0, 0, 0); 
	cv::Mat3b matBGR(colorVectorBGR);
	cv::Mat3b matHSV(colorVectorHSV);
	cv::cvtColor(matBGR, matHSV, cv::COLOR_BGR2HSV);
	colorVectorHSV = matHSV.at<cv::Vec3b>(0,0);

	// bounds based on percentage
	double hue = colorVectorHSV(0);
	double sat = colorVectorHSV(1);
	double val = colorVectorHSV(2);
	double lowerBoundHue = hue * (1 - deviationHue);
	double upperBoundHue = hue * (1 + deviationHue);
	double lowerBoundSat = sat * (1 - deviationSatVal);
	double upperBoundSat = sat * (1 + deviationSatVal);
	double lowerBoundVal = val * (1 - deviationSatVal);
	double upperBoundVal = val * (1 + deviationSatVal);
	cv::Scalar lowerBound(lowerBoundHue, lowerBoundSat, lowerBoundVal);
	cv::Scalar upperBound(upperBoundHue, upperBoundSat, upperBoundVal);
	
	// get binary mask based on color matching
	cv::Mat imageHSV;
	cv::cvtColor(imageBGR, imageHSV, cv::COLOR_BGR2HSV);
	cv::Mat binaryMask;
	cv::inRange(imageHSV, lowerBound, upperBound, binaryMask);

	// DEBUG 
	//imshow("mask", binaryMask);
	//cv::Mat binaryMaskColor;
	//cv::cvtColor(binaryMask, binaryMaskColor, cv::COLOR_GRAY2BGR);

	std::vector<cv::Rect> boundingBoxes;
	std::vector<std::vector<cv::Point>> contours;
	cv::findContours(binaryMask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
	for (unsigned int i = 0; i < contours.size(); ++i) { 
		cv::Rect bbox = cv::boundingRect(contours[i]);
		boundingBoxes.push_back(bbox);
		// DEBUG 
		//std::cout << i << " bbox size : " << bbox << std::endl;
		//std::cout << i << " bbox area : " << bbox.area() << std::endl << std::endl;
		//cv::rectangle(binaryMaskColor, bbox, green);
	}

	// DEBUG 
	//cv::imshow("contours", binaryMaskColor);

	return boundingBoxes;
}


void printTrackUpdate(cv::Mat trackImage, std::list<Track>* pDebugTracks) {
	cv::Scalar color[4] = {blue, green, red, yellow};
	int idx = 0;

	std::list<Track>::iterator iTrack = pDebugTracks->begin();
	while (iTrack != pDebugTracks->end()) {
		int confidence = iTrack->getConfidence();
        size_t id = iTrack->getId();
        int length = static_cast<int>(iTrack->getLength());
		//cv::Point2d vel2d = iTrack->getVelocity();
		double velocity = iTrack->getVelocity().x;

		cv::Rect rcActual = iTrack->getActualEntry().rect();
		cv::rectangle(trackImage, rcActual, color[id % 4], 2);
		
		cv::Rect rcPrev = iTrack->getPreviousEntry().rect();
		cv::rectangle(trackImage, rcPrev, color[id % 4]);

		std::stringstream ss;

		ss << "#" << id << ", con=" << confidence << ", len=" << length << ", v=" 
			<< std::fixed << std::setprecision(1) << velocity;

		// split status line, if necessary
		int font = cv::HersheyFonts::FONT_HERSHEY_SIMPLEX;
		double fontScale = 0.4;
		int baseline = 0;
		cv::Size textSize = cv::getTextSize(ss.str(), font, fontScale, 1, &baseline);

		int xOffset = 5;

		// fits onto one line 
		if (textSize.width + (xOffset * 2) < trackImage.size().width) {
			int yOffset = 10 + idx * 10;
			cv::putText(trackImage, ss.str(), cv::Point(xOffset, yOffset),
				font, fontScale, color[id % 4]);

		// two lines
		} else {
			size_t pos = std::string::npos;
			pos = ss.str().find(", len=");
			std::string line1 = ss.str().substr(0, pos);
			std::string line2 = ss.str().substr(pos + 2);
			int yOffset = 10 + idx * 20;
			cv::putText(trackImage, line1, cv::Point(xOffset, yOffset),
				font, fontScale, color[id % 4]);
			cv::putText(trackImage, line2, cv::Point(xOffset, yOffset + 10),
				font, fontScale, color[id % 4]);
		}

		++idx;
		++iTrack;
	}
}


cv::Rect scaleRect(cv::Rect rect, int scalingFactor) {
	cv::Point orgScaled = scalingFactor * rect.tl();
	cv::Size sizeScaled(scalingFactor * rect.width, scalingFactor * rect.height);
	cv::Rect scaled(orgScaled, sizeScaled);
	return scaled;
}


cv::Mat printTrackUpdateScaled(std::list<Track>* pDebugTracks, cv::Size roi, int scaling) {
	cv::Size roiScaled = roi * scaling;
	cv::Mat canvas(roiScaled, CV_8UC3, black);
	cv::Scalar color[4] = {blue, green, red, yellow};
	int idx = 0;

	std::list<Track>::iterator iTrack = pDebugTracks->begin();
	while (iTrack != pDebugTracks->end()) {
		int confidence = iTrack->getConfidence();
        size_t id = iTrack->getId();
        int length = static_cast<int>(iTrack->getLength());
		double velocity = iTrack->getVelocity().x;

		cv::Rect rcActual = scaleRect(iTrack->getActualEntry().rect(), scaling);
		cv::rectangle(canvas, rcActual, color[id % 4], 2);
		
		cv::Rect rcPrev = scaleRect(iTrack->getPreviousEntry().rect(), scaling);
		cv::rectangle(canvas, rcPrev, color[id % 4]);

		std::stringstream ss;

		ss << "#" << id << ", con=" << confidence << ", len=" << length << ", v=" 
			<< std::fixed << std::setprecision(1) << velocity;

		// split status line, if necessary
		int font = cv::HersheyFonts::FONT_HERSHEY_SIMPLEX;
		double fontScale = 0.4;
		int baseline = 0;
		cv::Size textSize = cv::getTextSize(ss.str(), font, fontScale, 1, &baseline);

		int xOffset = 5;

		// fits onto one line 
		if (textSize.width + (xOffset * 2) < canvas.size().width) {
			int yOffset = 10 + idx * 10;
			cv::putText(canvas, ss.str(), cv::Point(xOffset, yOffset),
				font, fontScale, color[id % 4]);

		// two lines
		} else {
			size_t pos = std::string::npos;
			pos = ss.str().find(", len=");
			std::string line1 = ss.str().substr(0, pos);
			std::string line2 = ss.str().substr(pos + 2);
			int yOffset = 10 + idx * 20;
			cv::putText(canvas, line1, cv::Point(xOffset, yOffset),
				font, fontScale, color[id % 4]);
			cv::putText(canvas, line2, cv::Point(xOffset, yOffset + 10),
				font, fontScale, color[id % 4]);
		}

		++idx;
		++iTrack;
	}
	return canvas;
}


cv::Mat readImage(std::string directory, std::string filePrefix, long long frameNumber) {
	std::string filePath = directory + filePrefix + "_" + std::to_string(frameNumber) + ".png";	
	cv::Mat image = cv::imread(filePath);
	return image;
}



bool setConfigRoiSize(Config* pConfig, std::string workPath, std::string filePrefix, long long startFrame) {
	using namespace std; 

	cv::Mat frame = readImage(workPath, filePrefix, startFrame);
	if (frame.empty()) {
		cerr << "was not able to read image file from path: " << workPath << endl;
		return false;
	}

    bool success = pConfig->setParam("roi_width", to_string(static_cast<long long>(frame.size().width)) );
    success &= pConfig->setParam("roi_height", to_string(static_cast<long long>(frame.size().height)) );

	if (success) {
		cout << "ROI set to: " << pConfig->getParam("roi_width") << "x" << pConfig->getParam("roi_height") << endl;
	} else {
		cerr << "was not able to set ROI correctly" << endl;
	}

	return success;
}



std:: string timeNow_YMD_HMS() {
	// get current time as tm struct
	time_t timeStampUnix = time(nullptr);
	struct tm* timeStamp;
	timeStamp = localtime(&timeStampUnix);

	std::stringstream timeString;
	timeString	<< 1900 + timeStamp->tm_year << "-" 
				<< 1 + timeStamp->tm_mon << "-"
				<< timeStamp->tm_mday << "_"
				<< timeStamp->tm_hour << "h_"
				<< timeStamp->tm_min  << "m_"
				<< timeStamp->tm_sec  << "s";

	return timeString.str();
}



bool trackImageSequence(SceneTracker* pScene, std::string directory, std::string filePrefix, long long start, long long stop, bool isManual) {
	bool isFileOutput = false;
	if (!isManual)
		isFileOutput = true;

	// make output directory with current time stamp

	std::string timeString = timeNow_YMD_HMS();
	std::string outputDir = directory + "/" + timeString;
	if (isFileOutput) {
		if (!makePath(outputDir)) {
			std::cerr << "cannot create: " << outputDir << std::endl;
			return false;
		}
	}

	//stop = 2;


	int idx = 0;
	for (; start <= stop; ++start) {
	
		cv::Mat input = readImage(directory, filePrefix, start);
		//cv::imshow("frame", input);
	
		std::vector<cv::Rect> motionRects = motionRectsFromDebugFrame(input, blue);
		// vector -> list (required input for SceneTracker.updateTracks);
		std::list<cv::Rect> motionRectList;
		for (unsigned int i = 0; i < motionRects.size(); ++i) {
			motionRectList.push_back(motionRects[i]);
		}

		// DEBUG
		cv::Mat debug(input.size(), CV_8UC3, black);
		for (unsigned int i = 0; i < motionRects.size(); ++i) { 
			cv::rectangle(debug, motionRects[i], green);
		}


		std::list<Track>* pDebugTracks;
		//pDebugTracks = pScene->updateTracks(motionRectList, start);
		pDebugTracks = pScene->updateTracks(motionRectList, idx);
		++idx;

		// show tracks
		cv::Size roi = input.size();
		cv::Mat canvas = printTrackUpdateScaled(pDebugTracks, roi, 2);

		// show occlusion
		/*
		if (pScene->isOverlappingTracks()) {
			const std::list<Occlusion>* pOcc = pScene->occlusionList();
			std::list<Occlusion>::const_iterator iOcc = pOcc->begin();
			while (iOcc != pOcc->end()) {
				cv::rectangle(tracks, iOcc->rect(), white);
				++iOcc;
			}
		}
		*/

		if (isManual) {
			cv::imshow("debug", debug);
			cv::imshow("tracks", canvas);
		}

		if (isFileOutput) {
			std::string filePath = outputDir + "\\" + "track" + "_" + std::to_string(start) + ".png";	
			cv::imwrite(filePath, canvas);
		}

		std::cout << "frame: " << start << " SPACE to continue, ESC to break" << std::endl << std::endl;
		if (cv::waitKey(0) == 27) {
			std::cout << "ESC pressed -> end sequence" << std::endl;
			break;
		}

	}
	
	return true;
}



int main(int argc, char* argv[]) {
    (void)argc; (void)argv;
	using namespace std;
	//string path320("D:/Users/Holger/counter/traffic320x240/");
	//string path640("D:/Users/Holger/counter/traffic640x480_low_pass/");
	string busStop("D:/Users/Holger/counter/2017-09-18/bus_stop/");
	string opposite("D:/Users/Holger/counter/2017-09-18/opposite/"); 
	int start = 630;
	int stop = 850;

	string workPath = findWorkPath(start, stop, busStop);
	if (workPath == "") {
		waitForEnter();
		return -1;
	}
	workPath += "/track debug/";
	string filePrefix = "debug";

	// collection of tracks and vehicles, generated by motion detection in frame_handler
	Config config;
	Config* pConfig = &config;

	// set config param roi according to image size in work path
	if (!setConfigRoiSize(&config, workPath, filePrefix, start)) {
		cerr << "roi size has not been set properly -> exit" << endl;
		waitForEnter();
		return -1;
	}

	SceneTracker scene(pConfig); 
	SceneTracker* pScene = &scene;
	config.attach(pScene);


	trackImageSequence(pScene, workPath, filePrefix, start, stop, false);

	cv::Size roi = getConfigRoiSize(pConfig);
	examineTrackState(g_trackState, roi);
	waitForEnter();
		
	return 0;
}
