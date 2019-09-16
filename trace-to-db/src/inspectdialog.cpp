#include "../inc/inspectdialog.h"

#include <QDebug>
#include <QGroupBox>
#include <QVBoxLayout>


InspectDialog::InspectDialog(QWidget *parent) : QDialog(parent)
{
    // ToDo vertical
    QGroupBox *groupRange = new QGroupBox("range", this);

    QLabel *frameRange = new QLabel("frame range");
    QLabel *videoOut = new QLabel("image processing");
    QLabel *trackInfo = new QLabel("track info");
    QVBoxLayout *topLayout = new QVBoxLayout();


    topLayout->addWidget(frameRange);
    topLayout->addWidget(videoOut);
    topLayout->addWidget(trackInfo);


    groupRange->setLayout(topLayout);
}

