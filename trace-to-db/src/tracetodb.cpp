#include "../inc/tracetodb.h"
#include "ui_tracetodb.h"
#include "clickablelabel.h"

#include <opencv2/opencv.hpp>
#include <QDebug>
#include <QFileDialog>
#include <QPainter>
#include "../../trace/inc/trackimages.h"

ClickableLabel* addClickableLabel(Ui::TraceToDb* ui) {
    int rowCount = ui->gridLayout_Video->rowCount();
    qDebug() << "rows in video layout:" << rowCount;
    ClickableLabel* label = new ClickableLabel();
    ui->gridLayout_Video->addWidget(label, rowCount+1, 0);
    return label;
}

TraceToDb::TraceToDb(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::TraceToDb)
{
    ui->setupUi(this);
    // m_videoLabel = addClickableLabel(ui);
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

    if (preview.isNull()) {
        //qDebug() << "empty preview image";
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

