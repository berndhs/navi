#include "map-display.h"


/****************************************************************
 * This file is distributed under the following license:
 *
 * Copyright (C) 2010, Bernd Stramm
 *
 *  This program is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public License
 *  as published by the Free Software Foundation; either version 2
 *  of the License, or (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, 
 *  Boston, MA  02110-1301, USA.
 ****************************************************************/


#include "deliberate.h"
#include "move-button.h"

#include <QPainter>
#include <QColor>
#include <QSize>

#include <QWheelEvent>
#include <QMouseEvent>

#include <QDebug>

using namespace deliberate;

namespace navi
{

MapDisplay::MapDisplay (QWidget *parent)
  :QWidget (parent),
   xLo (180.0),
   yLo (90.0),
   xHi (-180.0),
   yHi (-90.0),
   fullView (true),
   zoomView (false),
   followMouse (false),
   zoomScale (1.0)
{
  ui.setupUi (this);
  clock.start ();
  moveTimer = new QTimer (this);
  QString buttonStyle (
                   " QPushButton { "
                      " border: 2px solid #8f8f91; "
                       "   border-radius: 4px; "
                        "  background-color: rgba(255, 100, 100, 20); "
                           "}" );
  buttonStyle = Settings().value("mapstyle/buttonstyle", buttonStyle).toString();
  Settings().setValue ("mapstyle/buttonstyle",buttonStyle);
  ui.closeButton->setStyleSheet (buttonStyle);
  ui.zoomButton->setStyleSheet (buttonStyle);
  ui.fullButton->setStyleSheet (buttonStyle);
  qDebug () << " closeButton style sheet " << ui.closeButton->styleSheet();
  connect (ui.closeButton, SIGNAL (clicked()), this, SLOT (hide()));
  connect (ui.fullButton, SIGNAL (clicked()), this, SLOT (FullButton()));
  connect (ui.zoomButton, SIGNAL (clicked()), this, SLOT (ZoomButton()));
  connect (ui.moveButton, SIGNAL (Track(bool)), this, SLOT (MouseTrack (bool)));
  connect (moveTimer, SIGNAL (timeout()), this, SLOT (TimedMove ()));
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
MapDisplay::FullButton ()
{
  fullView = true;
  zoomView = false;
  zoomScale = 1.0;
  update ();
}

void
MapDisplay::ZoomButton ()
{
  fullView = false;
  zoomView = true;
  update ();
}

void
MapDisplay::MouseTrack (bool doTrack)
{
  followMouse = doTrack;
  setMouseTracking (doTrack);
  qDebug () << " Mouse tracking " << doTrack;
  if (followMouse) {
    moveTimer->start (100);
  } else {
    moveTimer->stop ();
  }
}

void
MapDisplay::paintEvent (QPaintEvent * event)
{
  if (fullView) {
    SetRange ();
    FullPaint ();
  } else if (zoomView) {
    FullPaint ();
  }
  if (ui.moveButton->Tracking ()) {
    QPainter painter;
    painter.begin (this);
    painter.setPen (QColor (0,0,0));
    QPoint mbcent = ui.moveButton->Center();
    QVector2D dir = ui.moveButton->Direction ().normalized();
    dir *= 60.0;
    painter.translate (mbcent.x(), mbcent.y());
    painter.drawLine (QPoint (0,0), dir.toPoint());
    painter.end ();
  }
  QWidget::paintEvent (event);
}

void
MapDisplay::wheelEvent (QWheelEvent * event)
{
  if (!zoomView) {
    return;
  }
  int  delta = event->delta();
  if (delta < 0) {
    zoomScale /= 1.2;
    update ();
  } else if (delta > 0) {
    zoomScale *= 1.2;
    update ();
  }
  QWidget::wheelEvent (event);
}

void
MapDisplay::mouseMoveEvent (QMouseEvent * event)
{
  if (followMouse) {
    qDebug () << " following mouse " << event;
  }
  QWidget::mouseMoveEvent (event);
}

void
MapDisplay::resizeEvent (QResizeEvent * event)
{
  int w = size().width();
  int h = size().height();
  ui.moveButton->CenterOn (QPoint(w/2,h/2));
  QWidget::resizeEvent (event);
}

void
MapDisplay::FullPaint ()
{
  QPainter painter;
  painter.begin (this);

  QSize canvasSize = size();
  int w = canvasSize.width();
  int h = canvasSize.height();

  painter.save ();
  painter.setPen (QColor(0,0,0,255));
  PaintPoints (&painter);
  painter.restore ();
 
  painter.end();
}

void
MapDisplay::ZoomPaint ()
{
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
  double x = (p.x() - xLo) * xScale * zoomScale;
  double y = (p.y() - yLo) * yScale * zoomScale;
  return QPointF (x,y);
}

void
MapDisplay::TimedMove ()
{
  QVector2D dir = ui.moveButton->Direction();
  double len = dir.length();
  dir.normalize();
  double moveLen (len > 70.0 ? 10.0 : 5.0);
  dir *= moveLen/(zoomScale * (xScale + yScale));
  double dx = dir.x();
  double dy = dir.y();
  xLo += dx;
  xHi += dx;
  yLo += dy;
  yHi += dy;
  update ();
}

} // namespace
