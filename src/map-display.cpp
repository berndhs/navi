#include "map-display.h"

#include <QPainter>
#include <QColor>
#include <QSize>

#include <QDebug>

namespace navi
{

MapDisplay::MapDisplay (QWidget *parent)
  :QWidget (parent),
   xLo (180.0),
   yLo (90.0),
   xHi (-180.0),
   yHi (-90.0)
{
  ui.setupUi (this);
  connect (ui.closeButton, SIGNAL (clicked()), this, SLOT (hide()));
}

void
MapDisplay::AddPoint (const QPointF & p)
{
  points.append (p);
  if (xLo > p.x()) { xLo = p.x(); }
  if (xHi < p.x()) { xHi = p.x(); }
  if (yLo > p.y()) { yLo = p.y(); }
  if (yHi < p.y()) { yHi = p.y(); }
}

void
MapDisplay::ClearPoints ()
{
  points.clear ();
  xLo = 180.0;
  yLo = 90.0;
  xHi = -180.0;
  yHi = -90.0;
}

void
MapDisplay::SetRange ()
{
  xRange = xHi - xLo;
  yRange = yHi - yLo;
  xScale = double (size().width()) / xRange;
  yScale = double (size().height())/ yRange;
}


void
MapDisplay::paintEvent (QPaintEvent * event)
{
qDebug () << "MapDisplay paint " << event;
  if (true || isVisible ()) {
qDebug () << " MapDisplay doing paintEvent";
    SetRange ();
    QPainter painter;
    painter.begin (this);

    QSize canvasSize = size();
    int w = canvasSize.width();
    int h = canvasSize.height();

    painter.save ();
    painter.setPen (QColor(0,0,0,255));
    PaintPoints (&painter);
    painter.restore ();
 
    painter.setPen (QColor(200,0,100,200));
    painter.translate (w/2,h/2);
    painter.drawLine (-30,0, 30,0);
    painter.drawLine (0,-30,0,30);
    painter.drawRect (-20, -20,40,40);
 
    painter.end();
  }
  QWidget::paintEvent (event);
}

void
MapDisplay::PaintPoints (QPainter * painter)
{
  if (!painter) {
    return;
  }
  int np = points.count();
  for (int i=0;i<np; i++) {
    QPointF here = Scale (points.at(i));
    painter->drawPoint (here);
  }
}

QPointF
MapDisplay::Scale (const QPointF p)
{
  double x = (p.x() - xLo) * xScale;
  double y = (p.y() - yLo) * yScale;
  return QPointF (x,y);
}

} // namespace
