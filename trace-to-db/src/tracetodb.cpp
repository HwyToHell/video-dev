#include "../inc/tracetodb.h"
#include "ui_tracetodb.h"

#include <opencv2/opencv.hpp>
#include <QFileDialog>
#include <QPainter>
#include "../../trace/inc/trackimages.h"

TraceToDb::TraceToDb(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::TraceToDb)
{
    ui->setupUi(this);
}

TraceToDb::~TraceToDb()
{
    delete ui;
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


void drawRectOnLabel(QLabel* label, QPixmap* pic) {
    QPainter p;
    p.begin(pic);
    p.setBrush(Qt::red);
    p.drawRect(10,10,100,100);
    p.end();

    label->setPixmap(*pic);
}


bool TraceToDb::setVideoPreviewImage(QPixmap preview) {
    // TODO set pixmap in label
    if (preview.isNull()) {
        //qDebug() << "empty preview image";
        ui->statusBar->showMessage("empty preview image");
        return false;
    }

    ui->video_output_label->setPixmap(preview);
    drawRectOnLabel(ui->video_output_label, &preview);
    return true;
}

