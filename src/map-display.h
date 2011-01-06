#ifndef NAVI_MAP_DISPLAY_H
#define NAVI_MAP_DISPLAY_H


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


#include "ui_map-display.h"

#include <QPointF>
#include <QList>
#include <QTime>
#include <QTimer>

#include <QVector2D>

class QPainter;

class QPaintEvent;
class QWheelEvent;
class QMouseMoveEvent;

namespace navi
{
class MapDisplay : public QWidget
{
Q_OBJECT

public:

  MapDisplay (QWidget * parent=0);

  void AddPoint (const QPointF & p, bool special=false);
  void ClearPoints ();

private slots:

  void FullButton ();
  void ZoomButton ();
  void MouseTrack (bool doTrack);
  void TimedMove ();

protected:

  void paintEvent (QPaintEvent * event);
  void wheelEvent (QWheelEvent * event);
  void mouseMoveEvent (QMouseEvent * event);
  void resizeEvent (QResizeEvent *event);

private:

  void PaintPoints (QPainter * painter, QList<QPointF> & plist);
  QPointF Scale (const QPointF p);
  void    SetRange ();

  void FullPaint ();
  void ZoomPaint ();

  Ui_MapDisplay   ui;
  QTime           clock;

  QList <QPointF>  points;
  QList <QPointF>  specialPoints;
  double           xLo;
  double           yLo;
  double           xHi;
  double           yHi;

  double           xRange;
  double           yRange;
  double           xScale;
  double           yScale;

  double           zoomScale;

  bool       fullView;
  bool       zoomView;
  bool       followMouse;

  QTimer    *moveTimer;

};

} // namespace


#endif