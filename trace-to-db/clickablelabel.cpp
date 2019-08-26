#include "clickablelabel.h"

#include <QDebug>
#include <QMouseEvent>
#include <QPainter>


QRect moveWithinLimits(const QRect& rcToMove, const QPoint& dPos, const QRect& rcLimits);



ClickableLabel::ClickableLabel(QWidget *parent) : QLabel(parent)
{

}


void ClickableLabel::mouseMoveEvent(QMouseEvent* event) {
    if (m_moveRoi) {
        // update click pos
        QPoint dPos = event->pos() - m_clickPosPrev;
        m_clickPosPrev = event->pos();

        // move rect
        QRect limits(QPoint(0,0), this->size());
        m_roi = moveWithinLimits(m_roiPrev, dPos, limits);
        m_roiPrev = m_roi;

        // emit paint event
        this->update();
        qDebug() << "dPos:" << dPos;
        qDebug() << "pos: " << m_roi;
    }
}


void ClickableLabel::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        // check if mouse pointer inside of roi
        if (m_roi.contains(event->pos())) {
            // enable moving roi
            // TODO change cursor
            m_clickPosPrev = event->pos();
            m_moveRoi = true;
            qDebug() << "left button pressed";
        }
    }
}


void ClickableLabel::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        m_moveRoi = false;
        qDebug() << "left button released";
    }
}


void ClickableLabel::paintEvent(QPaintEvent* event) {
    QLabel::paintEvent(event);

    QPainter painter;
    painter.begin(this);
    painter.setPen(Qt::red);
    painter.setCompositionMode(QPainter::RasterOp_SourceXorDestination);

    // moving: erase prev roi, draw new roi
    if (m_moveRoi) {
        //painter.drawRect(m_roiPrev);
        painter.drawRect(m_roi);
    // not moving: draw roi once
    } else {
        painter.drawRect(m_roi);
    }
    painter.end();
}



void ClickableLabel::setRoi(QRect roi) {
    m_roi = roi;
    m_roiPrev= roi;
    this->update();
}


///////////////////////////////////////////////////////////////////////////////
/// functions /////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

QRect moveWithinLimits(const QRect& rcToMove, const QPoint& dPos, const QRect& rcLimits) {
    QRect rcMoved(rcToMove.translated(dPos));

    // adjust horizontal position depending on horizontal bounds
    if (rcMoved.x() < rcLimits.x()) {
        rcMoved.setX(rcLimits.x());
        rcMoved.setWidth(rcToMove.width());
    } else {
        if (rcMoved.x() + rcMoved.width() > rcLimits.width()) {
            rcMoved.setX(rcLimits.width() - rcToMove.width());
            rcMoved.setWidth(rcToMove.width());
        }
    }

   // adjust vertical position depending on vertical bounds
    if (rcMoved.y() < rcLimits.y()) {
        rcMoved.setY(rcLimits.y());
        rcMoved.setHeight(rcToMove.height());
    } else {
        if (rcMoved.y() + rcMoved.height() > rcLimits.height()) {
            rcMoved.setY(rcLimits.height() - rcToMove.height());
            rcMoved.setHeight(rcToMove.height());
        }
    }
    qDebug() << "rcMoved:" << rcMoved;

    return rcMoved;
}


