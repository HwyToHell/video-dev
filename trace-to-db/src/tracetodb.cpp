#include "../inc/tracetodb.h"
#include "ui_tracetodb.h"
#include "clickablelabel.h"

#include <opencv2/opencv.hpp>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QPainter>
#include <QSettings>
#include <QVariant>
#include "../../trace/inc/trackimages.h"
#include "../../trace/inc/sql_trace.h"


ClickableLabel* addClickableLabel(Ui::TraceToDb* ui);
QRect getQRoiFromConfig(Config *pConfig);
bool isVideoFileAccessable(const QString& videoFile);
bool segmentToDb(FrameHandler *pFrameHandler, SceneTracker *pTracker, SqlTrace *pDB, const Inset& inset);
bool setQRoiToConfig(Config *pConfig, QRect roi);


TraceToDb::TraceToDb(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::TraceToDb)
{
    ui->setupUi(this);
    m_settingsFile = QApplication::applicationDirPath() + "/tracetodb.ini";
    loadSettings();

    // init config (from ~/counter/config.sqlite)
    m_config = std::make_unique<Config>();

    // segmentation done by frame handler
    m_framehandler = std::make_unique<FrameHandler>(m_config.get());
    m_config.get()->attach(m_framehandler.get());

    // tracker with unique IDs
    m_tracker = std::make_unique<SceneTracker>(m_config.get(), true);
    m_config.get()->attach(m_tracker.get());

    // set preview image (if available)
    if (isVideoFileAccessable(m_videoFile)) {
        ui->video_path_label->setText(m_videoFile);
        QPixmap preview = getPreviewImageFromVideo(m_videoFile);
        setVideoPreviewImage(preview);
        QRect roi = getQRoiFromConfig(m_config.get());
        ui->video_output_label->setRoi(roi);
    }
}


TraceToDb::~TraceToDb()
{
    saveSettings();
    delete ui;
}


void TraceToDb::loadSettings() {
    QSettings settings(m_settingsFile, QSettings::IniFormat);
    QVariant videoFileVariant = settings.value( "video_file", QDir::homePath() );
    if (videoFileVariant.isValid()) {
        m_videoFile = videoFileVariant.toString();
    }
}


void TraceToDb::on_runTrackingToDb_triggered()
{
    // save video file name and ROI to config before executing tracking
    //  framhandler and tracker are notified about parameter change by config::setParam
    QFileInfo fileInfo(m_videoFile);
    QString videoFilePath = fileInfo.absoluteFilePath();
    m_config->setParam("video_file", videoFilePath.toStdString());
    setQRoiToConfig(m_config.get(), ui->video_output_label->roi());
    m_config->saveConfigToFile();

    // create database in new directory (video file name w/o extension)
    QString dbPath = fileInfo.absolutePath() + "/" + fileInfo.completeBaseName();
    qDebug() << "db path:" << dbPath;
    QDir vidDir(fileInfo.absolutePath());
    if (!vidDir.mkpath(dbPath)) {
        QMessageBox msg;
        msg.setText("cannot create directory");
        return;
    }
    auto sqlTrace = std::make_unique<SqlTrace>(dbPath.toStdString(), "track.sqlite", "tracks");

    // create inset
    std::string insetImgPath = m_config->getParam("application_path");
    appendDirToPath(insetImgPath, m_config->getParam("inset_file"));
    Inset inset = m_framehandler->createInset(insetImgPath);

    // initialize file reader with selected video file
    bool success = m_framehandler->initFileReader(videoFilePath.toStdString());

    // execute segmentation
    success &= segmentToDb(m_framehandler.get(), m_tracker.get(), sqlTrace.get(), inset);

    if (success) {
        ui->statusBar->showMessage("tracking successful");
    } else {
        ui->statusBar->showMessage("error segmenting to DB");
    }


}


void TraceToDb::on_selectVideoFile_triggered()
{
   QString videoFile = QFileDialog::getOpenFileName(this,
        "Select video file",
        m_videoFile,
       "Video files (*.avi *.mp4)" );
    m_videoFile = videoFile;

    ui->video_path_label->setText(m_videoFile);

    // get label image
    QPixmap preview = getPreviewImageFromVideo(videoFile);
    setVideoPreviewImage(preview);

    // setROI on preview image label
    QRect roi = getQRoiFromConfig(m_config.get());
    ui->video_output_label->setRoi(roi);
}


void TraceToDb::saveSettings() {
    QSettings settings(m_settingsFile, QSettings::IniFormat);
    settings.setValue( "video_file", m_videoFile );
}


bool TraceToDb::setVideoPreviewImage(QPixmap preview) {

    if (preview.isNull()) {
        ui->statusBar->showMessage("empty preview image");
        return false;
    }
    ui->video_output_label->setPixmap(preview);

    // set fixed size, otherwise width will be larger than pixmap due to layout
    ui->video_output_label->setFixedSize(preview.size());

    return true;
}


//////////////////////////////////////////////////////////////////////////////
// Functions /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
ClickableLabel* addClickableLabel(Ui::TraceToDb* ui) {
    int rowCount = ui->gridLayout_Video->rowCount();
    ClickableLabel* label = new ClickableLabel();
    ui->gridLayout_Video->addWidget(label, rowCount+1, 0);
    return label;
}


QRect getQRoiFromConfig(Config *pConfig) {
    QRect roi;
    pConfig->readConfigFile(); // "~/counter/config.sqlite"
    roi.setX(std::stoi(pConfig->getParam("roi_x")));
    roi.setY(std::stoi(pConfig->getParam("roi_y")));
    roi.setWidth(std::stoi(pConfig->getParam("roi_width")));
    roi.setHeight(std::stoi(pConfig->getParam("roi_height")));
    // use default, if 0
    if (roi.x() == 0 || roi.y() == 0) {
        roi.setX(100);
        roi.setY(100);
    }
    return roi;
}


bool isVideoFileAccessable(const QString& videoFile) {
    QFileInfo fileInfo(videoFile);
    if (fileInfo.exists() && fileInfo.isReadable())
        return true;
    else
        return false;
}


bool segmentToDb(FrameHandler *pFrameHandler, SceneTracker *pTracker, SqlTrace *pDB, const Inset& inset) {

    bool success = false;

    try {

        long long frameCnt = 0;
        while(true)
        {
            ++frameCnt;

            if (!pFrameHandler->segmentFrame()) {
                std::cerr << "frame segmentation failed" << std::endl;
                break;
            }

        std::list<Track>* pTracks = pTracker->updateTracks(pFrameHandler->calcBBoxes(), frameCnt, pDB);
        pFrameHandler->showFrame(pTracks, inset);

            if (cv::waitKey(10) == 27) 	{
                std::cout << "ESC pressed -> end video processing" << std::endl;
                //cv::imwrite("frame.jpg", frame);
                break;
            }
        }

        qDebug() << "test finished";
        cv::waitKey(0);


    } catch (const char* e) {
        std::cerr << "exception: " << e << std::endl;
        success = false;
    }

    return success;

}


bool setQRoiToConfig(Config *pConfig, QRect roi) {
    bool success = pConfig->setParam("roi_x", std::to_string(roi.x()));
    success &= pConfig->setParam("roi_y", std::to_string(roi.y()));
    success &= pConfig->setParam("roi_width", std::to_string(roi.width()));
    success &= pConfig->setParam("roi_height", std::to_string(roi.height()));
    return success;
}
