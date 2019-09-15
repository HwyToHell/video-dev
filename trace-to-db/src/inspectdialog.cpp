#include "../inc/inspectdialog.h"

#include <QDebug>
#include <QVBoxLayout>


InspectDialog::InspectDialog(QWidget *parent) : QDialog(parent)
{
    // ToDo vertical
    QLabel *frameRange = new QLabel("frame range");
    QLabel *videoOut = new QLabel("image processing");
    QLabel *trackInfo = new QLabel("track info");
    QVBoxLayout *topLayout = new QVBoxLayout();

    topLayout->addWidget(frameRange);
    topLayout->addWidget(videoOut);
    topLayout->addWidget(trackInfo);

    this->setLayout(topLayout);
}

