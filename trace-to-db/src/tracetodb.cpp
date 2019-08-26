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


ClickableLabel* addClickableLabel(Ui::TraceToDb* ui);
void drawRectOnLabel(QLabel* label, QPixmap* pic);


TraceToDb::TraceToDb(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::TraceToDb)
{
    ui->setupUi(this);
    m_settingsFile = QApplication::applicationDirPath() + "/tracetodb.ini";
    loadSettings();
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
        m_workDir = videoFileVariant.toString();
    }
}


void TraceToDb::on_selectVideoFile_triggered()
{
   QString videoFile = QFileDialog::getOpenFileName(this,
        "Select video file",
        m_workDir,
       "Video files (*.avi *.mp4)" );
    m_videoFile = videoFile;

    ui->video_path_label->setText(m_videoFile);

    QPixmap preview = getPreviewImageFromVideo(videoFile);
    setVideoPreviewImage(preview);
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


    QRect roi(100, 100, 100, 100);
    ui->video_output_label->setRoi(roi);

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
