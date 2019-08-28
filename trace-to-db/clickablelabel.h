#ifndef CLICKABLELABEL_H
#define CLICKABLELABEL_H

#include <QLabel>

class ClickableLabel : public QLabel
{
    Q_OBJECT
public:
    explicit ClickableLabel(QWidget *parent = nullptr);
    QRect roi() const;
    void setRoi(QRect roi);

signals:

public slots:

protected:
    void mouseMoveEvent(QMouseEvent* event);
    void mousePressEvent(QMouseEvent* event);
    void mouseReleaseEvent(QMouseEvent* event);
    void paintEvent(QPaintEvent* event);

private:
    QRect m_roi;
    QRect m_roiPrev;
    QPoint m_clickPosPrev;
    bool m_moveRoi = false;

};

#endif // CLICKABLELABEL_H
