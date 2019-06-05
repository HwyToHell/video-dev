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

long extractNumber(const QString& fileName, const QString& prefix) {
    if (fileName.startsWith(prefix)) {
        int idxLeft = fileName.indexOf(QChar('_')) + 1;
        int idxRight = fileName.indexOf(QChar('.'));
        int length = idxRight - idxLeft;

        QString numberString = fileName.mid(idxLeft, length);
        qDebug() << numberString;
        return numberString.toLong();
    } else {
        return -1;
    }
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

    QMap<int, QString> map;

    QDirIterator itDir(inputDir);
    while(itDir.hasNext()) {
        qDebug() << itDir.next();
        qDebug() << itDir.fileName();
        long idx = extractNumber(itDir.fileName(), QString("debug"));
        qDebug() << idx;
        if (idx > 0) {
            map.insert(idx, itDir.fileName());
        }
    }

    //QTextStream out(stdout);
    //out << "map" << endl;

    qDebug() << "map debug";
    for (auto tuple: map.keys()) {
        qDebug() << tuple << ", " << map.value(tuple);
    }

}
