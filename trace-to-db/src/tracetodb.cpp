#include "../inc/tracetodb.h"
#include "ui_tracetodb.h"
#include "clickablelabel.h"

#include <opencv2/opencv.hpp>
#include <QDebug>
#include <QFileDialog>
#include <QPainter>
#include <QSettings>
#include <QVariant>
#include "../../trace/inc/trackimages.h"
#include "../../trace/inc/sql_trace.h"


ClickableLabel* addClickableLabel(Ui::TraceToDb* ui);
void drawRectOnLabel(QLabel* label, QPixmap* pic);
QRect getQRoiFromConfig(Config *pConfig);
bool segmentToDb(Config *pConfig, FrameHandler *pFrameHandler, SceneTracker *pTracker);
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
    // split video file name
    QFileInfo fileInfo(m_videoFile);
    //std::string videoFileName = fileInfo.fileName().toStdString();
    //std::string videoFileDir = fileInfo.path().toStdString();
    std::string videoFilePath = fileInfo.absoluteFilePath().toStdString();
    qDebug() << "name:" << fileInfo.absoluteFilePath();

    // save video file name and ROIto config before executing tracking
    m_config.get()->setParam("video_file", videoFilePath);
    setQRoiToConfig(m_config.get(), ui->video_output_label->roi());
    m_config.get()->saveConfigToFile();

    // update frame handler's and tracker's parameters from config
    m_framehandler.get()->update(); // for video file
    m_tracker.get()->update();      // for roi

    // execute segmentation
    // bool success = segmentToDb(Config *pConfig, FrameHandler *pFrameHandler, SceneTracker *pTracker);

}

void TraceToDb::on_selectVideoFile_triggered()
{
   QString videoFile = QFileDialog::getOpenFileName(this,
        "Select video file",
        m_videoFile,
       "Video files (*.avi *.mp4)" );
    m_videoFile = videoFile;

    ui->video_path_label->setText(m_videoFile);

    QPixmap preview = getPreviewImageFromVideo(videoFile);
    setVideoPreviewImage(preview);

    // setROI on preview image label
    QRect roi = getQRoiFromConfig(m_config.get());
    qDebug() << "roi:" << roi;
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


void drawRectOnLabel(QLabel* label, QPixmap* pic) {
    QPainter p;
    p.begin(pic);
    p.setBrush(Qt::red);
    p.drawRect(10,10,100,100);
    p.end();

    label->setPixmap(*pic);
}


QRect getQRoiFromConfig(Config *pConfig) {
    QRect roi;
    pConfig->readConfigFile(); // "~/counter/config.sqlite"
    roi.setX(std::stoi(pConfig->getParam("roi_x")));
    roi.setY(std::stoi(pConfig->getParam("roi_y")));
    roi.setWidth(std::stoi(pConfig->getParam("roi_width")));
    roi.setHeight(std::stoi(pConfig->getParam("roi_height")));
    return roi;
}


bool segmentToDb(Config *pConfig, FrameHandler *pFrameHandler, SceneTracker *pTracker) {

    try {
        // create inset
        std::string insetImgPath = pConfig->getParam("application_path");
        appendDirToPath(insetImgPath, pConfig->getParam("inset_file"));
        Inset inset = pFrameHandler->createInset(insetImgPath);

        // define directories for creating sql trace object
        std::string workDir ="/home/holger/counter";
        std::string dbFile = "track.sqlite";

        auto pSqlTrace = std::make_unique<SqlTrace>(workDir, dbFile, "tracks");
        std::cout << pSqlTrace.get() << std::endl;

        long long frameCnt = 0;
        while(true)
        {
            ++frameCnt;

            if (!pFrameHandler->segmentFrame()) {
                std::cerr << "frame segmentation failed" << std::endl;
                break;
            }

        std::list<Track>* pTracks = pTracker->updateTracks(pFrameHandler->calcBBoxes(), frameCnt, pSqlTrace.get());
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
    }

    return true;

}


bool setQRoiToConfig(Config *pConfig, QRect roi) {
    bool success = pConfig->setParam("roi_x", std::to_string(roi.x()));
    success &= pConfig->setParam("roi_y", std::to_string(roi.y()));
    success &= pConfig->setParam("roi_width", std::to_string(roi.width()));
    success &= pConfig->setParam("roi_height", std::to_string(roi.height()));
    return success;
}
