#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <iostream>
#include <QtWidgets>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}


// TODO implement directory validity check
//  - file naming convention ("debug" prefix)
//  - index ascending with no gap
//  - while (isValidDirectoryContent)

void MainWindow::on_actionSelect_Directory_triggered()
{
    QString inputDir = QFileDialog::getExistingDirectory(this,
        "Select directory with debug images",
        "D:/Users/Holger/counter", QFileDialog::ShowDirsOnly);

    qDebug() << inputDir;

    QDirIterator itDir(inputDir);
    while(itDir.hasNext()) {

        qDebug() << itDir.next();
        qDebug() << itDir.fileName();
    }
}
