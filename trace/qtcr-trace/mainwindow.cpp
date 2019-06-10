#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QtWidgets>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QVariant>

long extractNumber(const QString& fileName, const QString& prefix);

QMap<int, QString> populateFileMap(const QString& workDir);

void setLCDRed(QLCDNumber* pLCD);


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setLCDRed(ui->idx_actual);
    ui->next->setDisabled(true);
    ui->previous->setDisabled(true);

    m_settingsFile = QApplication::applicationDirPath() + "/trace.ini";
    loadSettings();
}


MainWindow::~MainWindow()
{
    saveSettings();
    delete ui;
}


long extractNumber(const QString& fileName, const QString& prefix) {
    if (fileName.startsWith(prefix)) {
        int idxLeft = fileName.indexOf(QChar('_')) + 1;
        int idxRight = fileName.indexOf(QChar('.'));
        int length = idxRight - idxLeft;

        QString numberString = fileName.mid(idxLeft, length);
        return numberString.toLong();

    } else {
        return -1;
    }
}


void MainWindow::loadSettings() {
    QSettings settings(m_settingsFile, QSettings::IniFormat);
    QVariant workDirVariant = settings.value( "work_dir", QDir::homePath() );
    if (workDirVariant.isValid()) {
        m_workDir = workDirVariant.toString();
    }
    qDebug() << "work dir: " << m_workDir;

    ui->workdir_output->setText(m_workDir);
}


void MainWindow::on_actionApply_Tracking_Algorithm_triggered()
{
    ui->statusBar->showMessage("start executing algorithm ...", 3000);
}


void MainWindow::on_actionRead_Contents_triggered()
{
    ui->statusBar->showMessage("reading directory contents ...", 3000);

    m_inputFiles = populateFileMap(m_workDir);

    if (!m_inputFiles.empty()) {
        ui->idx_begin->display(m_inputFiles.firstKey());
        ui->idx_end->display(m_inputFiles.lastKey());
        ui->next->setEnabled(true);
        ui->previous->setEnabled(true);

        m_itInputFile = m_inputFiles.begin();
        ui->idx_actual->display(m_itInputFile.key());
    } else {
        ui->statusBar->showMessage("no segmentation results in working directory");
    }

}


void MainWindow::on_actionSelect_triggered()
{
    QString inputDir = QFileDialog::getExistingDirectory(this,
        "Select directory with debug images",
        m_workDir, QFileDialog::ShowDirsOnly);
    m_workDir = inputDir;

    ui->workdir_output->setText(m_workDir);

    ui->statusBar->showMessage("directory changed, please read directory contents");

    /*
    qDebug() << "map debug";
    for (auto tuple: m_inputFiles.keys()) {
        qDebug() << tuple << ", " << m_inputFiles.value(tuple);
    }
    */
}


void MainWindow::on_next_clicked()
{
    if ( m_itInputFile == (m_inputFiles.cend()-1) )
        m_itInputFile = m_inputFiles.cbegin();
    else
        ++m_itInputFile;
    ui->idx_actual->display(m_itInputFile.key());
}


void MainWindow::on_previous_clicked()
{
    if ( m_itInputFile == m_inputFiles.cbegin() )
        m_itInputFile = (m_inputFiles.cend()-1);
    else
        --m_itInputFile;
    ui->idx_actual->display(m_itInputFile.key());
}


QMap<int, QString> populateFileMap(const QString& workDir) {
    QMap<int, QString> fileMap;
    QDirIterator itDir(workDir);
    while(itDir.hasNext()) {
        itDir.next();
        long idx = extractNumber(itDir.fileName(), QString("debug"));
        if (idx > 0) {
            fileMap.insert(idx, itDir.fileName());
        }
    }
    return fileMap;
}


void MainWindow::saveSettings() {
    QSettings settings(m_settingsFile, QSettings::IniFormat);
    settings.setValue( "work_dir", m_workDir );

    qDebug() << "work dir: " << m_workDir;
}


void setLCDRed(QLCDNumber* pLCD) {
    QPalette lcdRed;
    lcdRed.setColor(QPalette::WindowText, Qt::red);
    pLCD->setPalette(lcdRed);
}
