#ifndef TRACETODB_H
#define TRACETODB_H

#include<memory>
#include <QMainWindow>

#include "clickablelabel.h"
#include "../../car-count/include/config.h"
#include "../../car-count/include/frame_handler.h"
#include "../../car-count/include/tracker.h"

namespace Ui {
class TraceToDb;
}

class TraceToDb : public QMainWindow
{
    Q_OBJECT

public:
    explicit TraceToDb(QWidget *parent = nullptr);
    ~TraceToDb();

private slots:
    void on_selectVideoFile_triggered();

    void on_runTrackingToDb_triggered();

private: // functions
    void loadSettings();
    void saveSettings();
    bool setVideoPreviewImage(QPixmap preview);

private: // variables
    Ui::TraceToDb                   *ui;
    std::unique_ptr<Config>         m_config = nullptr;
    std::unique_ptr<FrameHandler>   m_framehandler = nullptr;
    std::unique_ptr<SceneTracker>   m_tracker = nullptr;
    QString                         m_settingsFile;
    QString                         m_videoFile;
    ClickableLabel                  *m_videoLabel; // TODO delete
    QString                         m_workDir = "/home/holger";
};

#endif // TRACETODB_H
