#include "stdafx.h"
#include <ctime>
#include <boost/filesystem.hpp>
#include "D:/Holger/app-dev/video-dev/car-count/include/frame_handler.h"
#include "D:/Holger/app-dev/video-dev/car-count/include/tracker.h"

#pragma warning(disable: 4482) // MSVC10: enum nonstd extension

/// read track debug frame from directory
cv::Mat readImage(std::string directory, std::string fileName, long long frameNumber);


/// read image sequence
bool trackImageSequence(SceneTracker* pScene, std::string directory, std::string file, long long start, long long stop, bool isManual = true);


/// extract motion rectangles with specified color from debug frame
std::vector<cv::Rect> motionRectsFromDebugFrame(cv::Mat imageBGR, cv::Scalar color,
	double deviationHue = 0.01, double deviationSatVal = 0.1);

void printTrackUpdate(cv::Mat trackImage, std::list<Track>* pDebugTracks);

std::string timeNow_YMD_HMS();

bool waitForEnter() {
	using namespace std;
	cout << endl << "Press <enter> to exit" << endl;
	string str;
	getline(cin, str);
	return true;
}


int trackLoop(int startFrame, int stopFrame, std::string workPath = "D:\\Users\\Holger\\counter\\traffic320x240\\");

int trackLoop(int startFrame, int stopFrame, std::string workPath) {
	using namespace boost::filesystem;
	using namespace std;

	// directory prefix to look for
	string dirPrefix = std::to_string((long long)startFrame) + " - " + std::to_string((long long)stopFrame);
	
	// list workPath content
	path workPathFS(workPath);
	cout << workPathFS << endl;

	if (exists(workPathFS)) {
		cout << "exists" << endl;
		if (is_directory(workPathFS)) {
			cout << "is directory" << endl;
		}
	}
	cout << endl;

	directory_iterator iDir(workPathFS);
	while (iDir != directory_iterator()) {
		cout << iDir->path().filename() << endl;
		string dir(iDir->path().filename());
		if (dir.find(dirPrefix) != string::npos) {
			// change dir
			workPath = iDir->path().generic_path();
			cout << workPath << endl;
		}
		++iDir;
	}


	return 0;
}

int main(int argc, char* argv[]) {
	using namespace std;

	trackLoop(0, 0);
	waitForEnter();
	//string directory = "D:\\Users\\Holger\\counter\\50 - 67 two cars opposite\\track debug\\";
	//string directory = "D:\\Users\\Holger\\counter\\151 - 175 car\\track debug\\";
	//string directory = "D:\\Users\\Holger\\counter\\260 - 274 two cars right\\track debug\\";
	//string directory = "D:\\Users\\Holger\\counter\\360 - 392 with trailer\\track debug\\";
	//string directory = "D:\\Users\\Holger\\counter\\440 - 455 skip truck\\track debug\\";
	//string directory = "D:\\Users\\Holger\\counter\\520 - 560 opposite\\track debug\\";
	string directory = "D:\\Users\\Holger\\counter\\740 - 800 opposite\\track debug\\";
	if (!isFileExist(directory)) {
		cout << "directory: " << directory << " does not exist" << endl;
		if(waitForEnter())
			return -1;
	}
	string filePrefix = "debug";

	// collection of tracks and vehicles, generated by motion detection in frame_handler
	Config config;
	Config* pConfig = &config;
	SceneTracker scene(pConfig); 
	SceneTracker* pScene = &scene;
	config.attach(pScene);

	trackImageSequence(pScene, directory, filePrefix, 0, 0, false);

	waitForEnter();
		
	return 0;
}



std::vector<cv::Rect> motionRectsFromDebugFrame(cv::Mat imageBGR, cv::Scalar colorBGR, 
	double deviationHue, double deviationSatVal) {
	
	// convert BGR to HSV color space for better separation based on single color value
	cv::Vec3b colorVectorBGR(colorBGR(0), colorBGR(1), colorBGR(2)); 
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
		int id = iTrack->getId();
		int length = iTrack->getLength();
		//cv::Point2d vel2d = iTrack->getVelocity();
		double velocity = iTrack->getVelocity().x;

		cv::Rect rcActual = iTrack->getActualEntry().rect();
		cv::rectangle(trackImage, rcActual, color[id % 4], 2);
		
		cv::Rect rcPrev = iTrack->getPreviousEntry().rect();
		cv::rectangle(trackImage, rcPrev, color[id % 4]);

		std::stringstream ss;
		int xOffset = 5;
		int yOffset = 10 + idx * 10;
		ss << "#" << id << ", con=" << confidence << ", len=" << length << ", vel=" 
			<< std::fixed << std::setprecision(1) << velocity;
		cv::putText(trackImage, ss.str(), cv::Point(xOffset, yOffset),
			cv::HersheyFonts::FONT_HERSHEY_SIMPLEX, 0.4, color[id % 4]);

		++idx;
		++iTrack;
	}
}


// TODO upgrade: find file containing a given number in directory and subs
cv::Mat readImage(std::string directory, std::string filePrefix, long long frameNumber) {
	std::string filePath = directory + filePrefix + "_" + std::to_string(frameNumber) + ".png";	
	cv::Mat image = cv::imread(filePath);
	return image;
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
	std::string outputDir = directory + timeString;
	if (!makePath(outputDir)) {
		std::cerr << "cannot create: " << outputDir << std::endl;
		return false;
	}



	for (; start <= stop; ++start) {
	
		cv::Mat input = readImage(directory, filePrefix, start);
		//cv::imshow("frame", input);
	
		std::vector<cv::Rect> motionRects = motionRectsFromDebugFrame(input, blue);
		// vector -> list (required input for SceneTracker.updateTracks);
		std::list<TrackEntry> motionRectList;
		for (unsigned int i = 0; i < motionRects.size(); ++i) {
			motionRectList.push_back(motionRects[i]);
		}

		// DEBUG
		cv::Mat debug(input.size(), CV_8UC3, black);
		for (unsigned int i = 0; i < motionRects.size(); ++i) { 
			cv::rectangle(debug, motionRects[i], green);
		}


		std::list<Track>* pDebugTracks;
		pDebugTracks = pScene->updateTracksIntersect(motionRectList, start);
		// DEBUG
		cv::Mat tracks(input.size(), CV_8UC3, black);
		printTrackUpdate(tracks, pDebugTracks);

		if (isManual) {
			cv::imshow("debug", debug);
			cv::imshow("tracks", tracks);
		}

		if (isFileOutput) {
			std::string filePath = outputDir + "\\" + "track" + "_" + std::to_string(start) + ".png";	
			cv::imwrite(filePath, tracks);
		}

		std::cout << "frame: " << start << " SPACE to continue, ESC to break" << std::endl;
		if (cv::waitKey(0) == 27) {
			std::cout << "ESC pressed -> end sequence" << std::endl;
			break;
		}

	}
	

	return true;

}
