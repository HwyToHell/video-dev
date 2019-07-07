#include "mainwindow.h"
#include "ui_mainwindow.h"


#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QVariant>

#include "trackimages.h"
#include "D:/Holger/app-dev/video-dev/utilities/inc/util-visual-trace.h"


long extractNumber(const QString& fileName, const QString& prefix);

QMap<int, QString> populateFileMap(const QString& workDir);

void setLCDRed(QLCDNumber* pLCD);

void showBlobs(Ui::MainWindow* uimain, const QSize& imgSize);

void showTracks();


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

    // if no tracking results -> don't show keys
    if (isTraceValid()) {
        ui->idx_begin->display(minIdxTrackState());
        ui->idx_end->display(maxIdxTrackState());
        ui->idx_actual->display(minIdxTrackState());

        ui->next->setEnabled(true);
        ui->previous->setEnabled(true);

        ui->trace_desc_1->setText(QString("Blobs"));
        showBlobs(ui, m_imgSize);

        // add widgets depending on lenght of TrackState list
        for (auto trackState: g_trackStateMap.begin()->second) {
            TraceVisu traceVisu;

            // create description label
            QString labelText(QString::fromStdString(trackState.m_name));
            qDebug() << labelText;
            traceVisu.description = new QLabel(labelText);

            // create picture label
            traceVisu.picture = new QLabel();
            traceVisu.picture->setPixmap(getTrackImage(trackState, m_imgSize));

            // save label pointers and add widgets
            m_traceVisu.push_back(traceVisu);
            int column = m_traceVisu.size();
            ui->gridLayout_5->addWidget(traceVisu.description, 0, column);
            ui->gridLayout_5->addWidget(traceVisu.picture, 1, column);
        }

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


    // TODO: use for dynamically added widgets
    //QList<QPixmap> imgList = getCurrImgList(m_imgSize);
    //ui->trace_image_1->setPixmap(imgList.front());



    // DEBUG show traversing over map
    /*
    if ( m_itInputFile == (m_inputFiles.cend()-1) )
        m_itInputFile = m_inputFiles.cbegin();
    else
        ++m_itInputFile;
    ui->idx_actual->display(m_itInputFile.key());
    */
}


void MainWindow::on_previous_clicked()
{
    // lower index of g_trackStateMap
    int idx = prevTrackState();
    ui->idx_actual->display(idx);

    showBlobs(ui, m_imgSize);

    // TODO: use for dynamically added widgets
    //QList<QPixmap> imgList = getCurrImgList(m_imgSize);
    //ui->trace_image_1->setPixmap(imgList.front());

    // DEBUG show traversing over map
    /*
    if ( m_itInputFile == m_inputFiles.cbegin() )
        m_itInputFile = (m_inputFiles.cend()-1);
    else
        --m_itInputFile;
    ui->idx_actual->display(m_itInputFile.key());
    */
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
        long idx = extractNumber(itDir.fileName(), QString("debug"));
        if (idx > 0) {
            fileMap.insert(idx, itDir.fileName());
        }
    }
    return fileMap;
}


void setLCDRed(QLCDNumber* pLCD) {
    QPalette lcdRed;
    lcdRed.setColor(QPalette::WindowText, Qt::red);
    pLCD->setPalette(lcdRed);
}

/// show blob image in label trace_image_1
void showBlobs(Ui::MainWindow* uimain, const QSize& imgSize) {
    QPixmap blobImage = getCurrBlobImage(imgSize);
    uimain->trace_image_1->setPixmap(blobImage);
}

// TODO implement iteration through widget list
void showTracks() {}
