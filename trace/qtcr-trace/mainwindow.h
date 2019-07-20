#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>
#include <QtWidgets>

#if defined(__linux__)
    #include "../../car-count/include/config.h"
    #include "../../car-count/include/tracker.h"
#elif(_WIN32)
    #include "D:/Holger/app-dev/video-dev/car-count/include/config.h"
    #include "D:/Holger/app-dev/video-dev/car-count/include/tracker.h"
#endif



namespace Ui {
class MainWindow;
}

struct TraceLabels{
    QLabel* description;
    QLabel* picture;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void loadSettings();
    void readDirContents();
    void saveSettings();

private slots:
    void on_actionApply_Tracking_Algorithm_triggered();

    void on_actionRead_Contents_triggered();

    void on_actionSelect_triggered();

    void on_next_clicked();

    void on_previous_clicked();

private:
    Ui::MainWindow *ui;
    int m_idxActual;
    int m_idxBegin;
    int m_idxEnd;
    QSize m_imgSize;
    QMap<int, QString> m_inputFiles;
    QMap<int, QString>::const_iterator m_itInputFile;
    QString m_settingsFile;
    QString m_workDir;
    QList<TraceLabels> m_traceLabelList;
    std::unique_ptr<Config> m_pConfig;
    std::unique_ptr<SceneTracker> m_pTracker;
};

#endif // MAINWINDOW_H
