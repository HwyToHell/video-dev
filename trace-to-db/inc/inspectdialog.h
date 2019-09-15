#ifndef INSPECTDIALOG_H
#define INSPECTDIALOG_H

#include <QDialog>
#include <QLabel>

class InspectDialog : public QDialog
{
    Q_OBJECT
public:
    explicit InspectDialog(QWidget *parent = nullptr);

signals:

public slots:

protected:

private:
    QLabel *m_frameRange;
    QLabel *m_videoOut;
    QLabel *m_trackInfo;

};

#endif // INSPECTDIALOG_H
