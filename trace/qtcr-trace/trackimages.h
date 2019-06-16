#ifndef TRACKIMAGES_H
#define TRACKIMAGES_H
#include <QMap>
#include <QString>

#include "D:/Holger/app-dev/video-dev/car-count/include/tracker.h"


// put tracking results into g_trackState (defined in util-visual-trace.cpp)
bool trackImages(const QString& directory, QMap<int, QString> imgFiles, SceneTracker* pScene);

/// extract motion rectangles with specified color from debug frame
std::list<cv::Rect> motionRectsFromDebugImage(cv::Mat imageBGR, cv::Scalar color,
    double deviationHue = 0.01, double deviationSatVal = 0.1);


#endif // TRACKIMAGES_H
