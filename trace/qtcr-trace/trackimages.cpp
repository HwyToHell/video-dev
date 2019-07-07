#include "trackimages.h"
#include <cmath>
#include <iostream>
#include <list>
#include <string>
#include <opencv2/opencv.hpp>
#include <QDebug>
#include <QList>

#include "D:/Holger/app-dev/video-dev/car-count/include/config.h"
#include "D:/Holger/app-dev/video-dev/car-count/include/frame_handler.h"
#include "D:/Holger/app-dev/video-dev/utilities/inc/util-visual-trace.h"


QImage cvMatToQImage(const cv::Mat& inMat) {
    QImage image( inMat.data, inMat.cols, inMat.rows, static_cast<int>(inMat.step),
                    QImage::Format_RGB888 );
      return image.rgbSwapped();
}


QPixmap cvMatToQPixmap(const cv::Mat& inMat) {
    return QPixmap::fromImage( cvMatToQImage( inMat ) );
}


QSize getRoiSize(QString fileName) {
    QSize size(0,0);
    cv::Mat image = cv::imread(fileName.toStdString());
    if (!image.empty()) {
        size.setWidth(image.size().width);
        size.setHeight(image.size().height);
    }
    return size;
}


const std::list<TrackState>* getTrackState() {
    return &g_itCurrent->second;
}


int numberOfTrackStates() {
    return static_cast<int>(g_itCurrent->second.size());
}

bool isTraceValid() {
    if (g_trackStateMap.size() > 0)
        return true;
    else
        return false;
}


int maxIdxTrackState() {
    return static_cast<int>( (--g_trackStateMap.cend())->first );
}


int minIdxTrackState() {
    return static_cast<int>( g_trackStateMap.cbegin()->first );
}


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


QList<QPixmap> getCurrImgList(QSize dispImgSize) {
    QList<QPixmap> imgList;
    cv::Size dispSize(dispImgSize.width(), dispImgSize.height());
    for (auto trackStateList: g_itCurrent->second) {
        cv::Mat canvas(dispSize, CV_8UC3, black);
        printTracksScaled(canvas, trackStateList.m_tracks, trackStateList.m_roi, true);
        QPixmap pic = cvMatToQPixmap(canvas);
        imgList.push_back(pic);
    }
    return imgList;
}

QPixmap getCurrBlobImage(QSize dispImgSize) {
    cv::Size dispSize(dispImgSize.width(), dispImgSize.height());
    cv::Mat canvas(dispSize, CV_8UC3, black);

    // take blobs from first track state
    TrackState trackState(g_itCurrent->second.front());
    printBlobsScaled(canvas, trackState.m_blobs, trackState.m_roi);

    QPixmap blobImage = cvMatToQPixmap(canvas);
    return blobImage;
}


QPixmap getTrackImage(const TrackState& trackState, QSize dispImgSize) {
    cv::Size dispSize(dispImgSize.width(), dispImgSize.height());
    cv::Mat canvas(dispSize, CV_8UC3, black);

    printTracksScaled(canvas, trackState.m_tracks, trackState.m_roi, true);
    QPixmap trackImage = cvMatToQPixmap(canvas);
    return trackImage;
}


int nextTrackState() {
    ++g_itCurrent;
    // tail reached -> set iterator to head
    if ( g_itCurrent == g_trackStateMap.cend()  )
        g_itCurrent = g_trackStateMap.cbegin();
    //qDebug() << "idx:" << g_itCurrent->first;
    return static_cast<int>( g_itCurrent->first );
}


int prevTrackState() {
    if ( g_itCurrent != g_trackStateMap.cbegin()  )
        --g_itCurrent;
    // head reached -> set iterator to tail
    else
        g_itCurrent = --g_trackStateMap.end();
    //qDebug() << "idx:" << g_itCurrent->first;
    return static_cast<int>( g_itCurrent->first );
}


void setRoiToImgSize(Config* pConfig, QString workDir, QString file) {
    std::string fileName = workDir.toStdString() + "/" + file.toStdString();
    cv::Mat img = cv::imread(fileName);
    setRoiToConfig(pConfig, img.size());
}


bool trackImages(const QString& directory, QMap<int, QString> imgFiles, SceneTracker* pScene) {
    using namespace std;
    bool success = false;

    foreach(int iKey, imgFiles.keys()) {
        // DEBUG show files
        // qDebug() << iKey << "," << imgFiles.value(iKey);

        // read images to mat
        std::string fileName = directory.toStdString() + "/" + imgFiles.value(iKey).toStdString();
        //cout << fileName << endl;

        cv::Mat image = cv::imread(fileName);
        if (!image.empty()){
            // apply segmentation
            // check, if image not empty
            std::list<cv::Rect> motionRectList = motionRectsFromDebugImage(image, blue);
            std::list<Track>* pDebugTracks;
            pDebugTracks = pScene->updateTracks(motionRectList, iKey);
            (void)pDebugTracks;

            // DEBUG show tracks
            /*
            cv::Mat canvas(image.size(), CV_8UC3, black);
            printTracks(canvas, *pDebugTracks);
            cv::imshow("tracks", canvas);
            if (cv::waitKey(0) == 27) {
                std::cout << "ESC pressed -> end sequence" << std::endl;
                break;
            }
            */
            success = true;
        } else {
            success = false;
            break;
        }
    }
    // g_trackStateMap not empty
    //qDebug( "track state size: %i", g_trackStateMap.size() );
    if ( g_trackStateMap.size() == 0 ) {
        success = false;
        cerr << "track state empty" << endl;
    } else {
        // set first index as active
        g_itCurrent = g_trackStateMap.cbegin();
    }
    return success;
}
