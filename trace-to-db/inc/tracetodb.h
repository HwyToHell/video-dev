#ifndef TRACETODB_H
#define TRACETODB_H

#include <QMainWindow>

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

private: // functions
    bool setVideoPreviewImage(QPixmap preview);

private: // variables
    Ui::TraceToDb *ui;
    QString m_videoFile;
    QString m_workDir = "/home/holger";
};

#endif // TRACETODB_H
