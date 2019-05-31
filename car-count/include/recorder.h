#pragma once
#include <opencv2/opencv.hpp>


struct CountResults {
	int carLeft;
	int carRight;
	int truckLeft;
	int truckRight;
	CountResults() : carLeft(0), carRight(0), truckLeft(0), truckRight(0) {}
	CountResults operator+=(const CountResults& rhs);
	void printResults();
};


struct ClassifyVehicle {
	int countConfidence;
	int countPos;
	int trackLength;
	cv::Size2i truckSize;
};


class CountRecorder {
	int mCarCntLeft;
	int mCarCntRight;
	int mTruckCntLeft;
	int mTruckCntRight;
public:
	CountRecorder();
	CountRecorder(CountResults cr);
	CountResults getStatus();
	void updateCnt(bool movesLeft, bool isTruck = 0);
	void printResults();
};
