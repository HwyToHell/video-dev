#ifndef TRACKIMAGES_H
#define TRACKIMAGES_H
#include <QImage>
#include <QPixmap>
#include <QMap>
#include <QSize>
#include <QString>

#include "D:/Holger/app-dev/video-dev/car-count/include/tracker.h"


QImage cvMatToQImage(const cv::Mat& inMat);

QPixmap cvMatToQPixmap(const cv::Mat& inMat);

QList<QPixmap> getCurrImgList(QSize dispImgSize);

QSize getRoiSize(QString fileName);


const std::list<TrackState>* getTrackState();

bool isTraceValid();

int maxIdxTrackState();

int minIdxTrackState();

/// extract motion rectangles with specified color from debug frame
std::list<cv::Rect> motionRectsFromDebugImage(cv::Mat imageBGR, cv::Scalar color,
    double deviationHue = 0.01, double deviationSatVal = 0.1);

// advance idx to next track state (ringbuffer)
int nextTrackState();

int numberOfTrackStates();

// lower idx to previous track state (ringbuffer)
int prevTrackState();

// adjust roi size in config to image file size
void setRoiToImgSize(Config* pConfig, QString workDir, QString file);

// put tracking results into g_trackState (defined in util-visual-trace.cpp)
bool trackImages(const QString& directory, QMap<int, QString> imgFiles, SceneTracker* pScene);

#endif // TRACKIMAGES_H
