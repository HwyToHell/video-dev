#include "trackimages.h"
#include <cmath>
#include <iostream>
#include <list>
#include <string>
#include <opencv2/opencv.hpp>
#include <QDebug>

#include "D:/Holger/app-dev/video-dev/car-count/include/config.h"
#include "D:/Holger/app-dev/video-dev/car-count/include/frame_handler.h"
#include "D:/Holger/app-dev/video-dev/utilities/inc/util-visual-trace.h"


std::list<cv::Rect> motionRectsFromDebugImage(cv::Mat imageBGR, cv::Scalar colorBGR,
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

    std::list<cv::Rect> boundingBoxes;
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







bool trackImages(const QString& directory, QMap<int, QString> imgFiles, SceneTracker* pScene) {
    bool success = false;
    qDebug() << "foreach";
    int idx = 0;
    foreach(int iKey, imgFiles.keys()) {
        qDebug() << iKey << "," << imgFiles.value(iKey);

        // read images to mat
        std::string fileName = directory.toStdString() + "/" + imgFiles.value(iKey).toStdString();
        using namespace std;
        cout << fileName << endl;

        cv::Mat image = cv::imread(fileName);
        if (!image.empty()){
            // apply segmentation
            // check, if image not empty
            std::list<cv::Rect> motionRectList = motionRectsFromDebugImage(image, blue);
            std::list<Track>* pDebugTracks;
            pDebugTracks = pScene->updateTracks(motionRectList, idx);
            ++idx;

            // show tracks
            cv::Mat canvas(image.size(), CV_8UC3, black);
            printTracks(canvas, *pDebugTracks);
            cv::imshow("tracks", canvas);
            if (cv::waitKey(0) == 27) {
                std::cout << "ESC pressed -> end sequence" << std::endl;
                break;
            }
            success = true;
        } else {
            success = false;
            break;
        }
    }
    return success;
}


