#include <cmath>
#include <iostream>
#include <list>
#include <string>


#include <opencv2/opencv.hpp>

#include "D:/Holger/app-dev/video-dev/car-count/include/config.h"
#include "D:/Holger/app-dev/video-dev/utilities/inc/util-visual-trace.h"

#include "trackimages.h"

void trackImages(QMap<int, QString> imgFiles, SceneTracker* pScene) {
    for (auto tuple: imgFiles.keys()) {
        qDebug() << tuple << ", " << imgFiles.value(tuple);
    }
}
