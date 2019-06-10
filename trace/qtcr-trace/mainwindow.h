#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMap>

namespace Ui {
class MainWindow;
}

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
    QMap<int, QString> m_inputFiles;
    QMap<int, QString>::const_iterator m_itInputFile;
    QString m_settingsFile;
    QString m_workDir;
};

#endif // MAINWINDOW_H
