#include "../inc/mainwindow.h"
#include "ui_mainwindow.h"


#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QVariant>

#include "../inc/trackimages.h"
#if defined(__linux__)
    #include "../../utilities/inc/util-visual-trace.h"
#elif(_WIN32)
    #include "D:/Holger/app-dev/video-dev/utilities/inc/util-visual-trace.h"
#endif


long extractNumber(const QString& fileName, const QString& prefix);

QMap<int, QString> populateFileMap(const QString& workDir);

void setTraceLayout(QGridLayout* grid, QList<TraceLabels>& traceLabelList,
               const MapTrackStateList& trackStateMap);

void setLCDRed(QLCDNumber* pLCD);

void showBlobs(Ui::MainWindow* uimain, const QSize& imgSize);

bool showTracks(Ui::MainWindow* uimain, const QSize& imgSize);

void showTrackInfo(Ui::MainWindow* uimain);

//////////////////////////////////////////////////////////////////////////////
// MainWindow ////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    // disable some gui elements, until valid settings have been loaded
    ui->setupUi(this);
    setLCDRed(ui->idx_actual);
    ui->next->setDisabled(true);
    ui->previous->setDisabled(true);
    ui->menuApply_Tracking->setDisabled(true);

    // default image size 200 x 200
    m_imgSize = QSize(200, 200);
    // load settings (ini file method on all platforms)
    m_settingsFile = QApplication::applicationDirPath() + "/trace.ini";
    loadSettings();
    readDirContents();

    // init tracker
    m_pConfig = std::unique_ptr<Config>(new Config);
    m_pTracker = std::unique_ptr<SceneTracker>(new SceneTracker(m_pConfig.get()));
}


MainWindow::~MainWindow()
{
    saveSettings();
    delete ui;
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
    QMap<int, QString> inputFiles = m_inputFiles;

    // set config->roi to image size, update SceneTracker
    setRoiToImgSize(m_pConfig.get(), m_workDir, inputFiles.first());
    m_pTracker.get()->update();

    // show message if reading error
    if ( !trackImages(m_workDir, inputFiles, m_pTracker.get()) )
        ui->statusBar->showMessage("error reading segmentation image");

    // enable keys only for valid if tracking results
    if (isTraceValid()) {
        ui->idx_begin->display(minIdxTrackState());
        ui->idx_end->display(maxIdxTrackState());
        ui->idx_actual->display(minIdxTrackState());
        ui->next->setEnabled(true);
        ui->previous->setEnabled(true);

        // adjust trace layout to length of tracking result data
        ui->trace_desc_1->setText(QString("Blobs"));
        setTraceLayout(ui->gridLayout_5, m_traceLabelList, g_trackStateMap);

        // show blob image in label trace_image_1
        showBlobs(ui, m_imgSize);
        showTracks(ui, m_imgSize);
        showTrackInfo(ui);

    // no valid tracking results
    } else {
        ui->statusBar->showMessage("no segmentation results in working directory");
    }
}


void MainWindow::on_actionRead_Contents_triggered()
{
    readDirContents();
}


void MainWindow::on_actionSelect_triggered()
{
    QString inputDir = QFileDialog::getExistingDirectory(this,
        "Select directory with debug images",
        m_workDir, QFileDialog::ShowDirsOnly);
    m_workDir = inputDir;

    ui->workdir_output->setText(m_workDir);
    readDirContents();
}


void MainWindow::on_next_clicked()
{
    // advance index of g_trackStateMap
    int idx = nextTrackState();
    ui->idx_actual->display(idx);

    // show blob image in label trace_image_1
    showBlobs(ui, m_imgSize);
    showTracks(ui, m_imgSize);
    showTrackInfo(ui);
}


void MainWindow::on_previous_clicked()
{
    // lower index of g_trackStateMap
    int idx = prevTrackState();
    ui->idx_actual->display(idx);

    showBlobs(ui, m_imgSize);
    showTracks(ui, m_imgSize);
    showTrackInfo(ui);
}


 void MainWindow::readDirContents() {
    ui->statusBar->showMessage("reading directory contents ...", 3000);

    m_inputFiles = populateFileMap(m_workDir);

    if (!m_inputFiles.empty()) {
        ui->menuApply_Tracking->setEnabled(true);
    } else {
        ui->statusBar->showMessage("no segmentation results in working directory");
    }
}


void MainWindow::saveSettings() {
    QSettings settings(m_settingsFile, QSettings::IniFormat);
    settings.setValue( "work_dir", m_workDir );
}


//////////////////////////////////////////////////////////////////////////////
// Functions /////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////

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


QMap<int, QString> populateFileMap(const QString& workDir) {
    QMap<int, QString> fileMap;
    QDirIterator itDir(workDir);
    while(itDir.hasNext()) {
        itDir.next();
        int idx = static_cast<int>(extractNumber(itDir.fileName(), QString("debug")));
        if (idx > 0) {
            fileMap.insert(idx, itDir.fileName());
        }
    }
    return fileMap;
}

void setTraceLayout(QGridLayout* grid, QList<TraceLabels>& traceLabelList,
               const MapTrackStateList& mapTrackStateList) {
    // column 0      : blobs
    // column 1 ... n: tracks

    // add / remove labels depepending on track state list length
    int nStatesNew = static_cast<int>(mapTrackStateList.begin()->second.size());
    int nStatesOld = traceLabelList.size();
    qDebug() << "new:" << nStatesNew;
    qDebug() << "old:" << nStatesOld;

    if (nStatesNew !=nStatesOld) {

        // add labels
        if (nStatesNew > nStatesOld) {
        qDebug() << "add";
            for (int state = nStatesOld; state < nStatesNew; ++state) {
                TraceLabels labels;
                qDebug() << "col:" << state;
                labels.description = new QLabel();
                labels.picture = new QLabel();
                grid->addWidget(labels.description, 0, state+1);
                grid->addWidget(labels.picture, 1, state+1);
                traceLabelList.append(labels);
            }

        // remove labels
        } else {
        qDebug() << "remove";
            for (int state = nStatesOld; state > nStatesNew; --state) {
                TraceLabels labels = traceLabelList.last();
                qDebug() << "col:" << state;
                // remove widget and delete pointer to it afterwards
                // https://stackoverflow.com/questions/11599273/qt-removewidget-and-object-deletion
                grid->removeWidget(labels.description);
                grid->removeWidget(labels.picture);
                delete labels.description;
                delete labels.picture;
                traceLabelList.removeLast();
            }
        }
    }

    // set trace visu description
    int column = 1;
    for (auto trackState : mapTrackStateList.begin()->second) {
        // iterate over labels in gridLayout and set label text
        QWidget* label = grid->itemAtPosition(0, column)->widget();
        static_cast<QLabel*>(label)->setText(QString::fromStdString(trackState.m_name));
        ++column;
    }
}


void setLCDRed(QLCDNumber* pLCD) {
    QPalette lcdRed;
    lcdRed.setColor(QPalette::WindowText, Qt::red);
    pLCD->setPalette(lcdRed);
}

// show blob image in label trace_image_1
void showBlobs(Ui::MainWindow* uimain, const QSize& imgSize) {
    QPixmap blobImage = getCurrBlobImage(imgSize);
    uimain->trace_image_1->setPixmap(blobImage);
}

// show track images in label-columns 1 ...n, label-row 1
bool showTracks(Ui::MainWindow* uimain, const QSize& imgSize) {
    QList<QPixmap> trackImageList = getCurrImgList(imgSize);
    // column 1 ... n: tracks
    int column = 1;
    // iterate over labels in gridLayout and set label pixmap
    for (auto pixmap : trackImageList) {
        QWidget* label = uimain->gridLayout_5->itemAtPosition(1, column)->widget();
        if (label == nullptr)
            return false;
        static_cast<QLabel*>(label)->setPixmap(pixmap);
        ++column;
    }
    return true;
}

void showTrackInfo(Ui::MainWindow* uimain) {
    QList<QString> infoList = getCurrTrackInfo();
    QString multiLine;
    for (auto line : infoList) {
        multiLine.append(line);
        multiLine.append('\n');
    }
    int lastChar = multiLine.size() - 1;
    multiLine.remove(lastChar, 1);
    uimain->param_output->setText(multiLine);
}
