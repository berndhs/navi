#include "move-button.h"



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


#include <QDebug>

#include <QPainter>
#include <QCursor>

namespace navi
{
MoveButton::MoveButton (QWidget *parent)
  :QPushButton (parent),
   parentWidget (parent),
   trackingMouse (false)
{
  Init ();
}

MoveButton::MoveButton ( const QString & text, QWidget * parent)
  :QPushButton (text, parent),
   parentWidget (parent),
   trackingMouse (false)
{
  Init ();
}

MoveButton::MoveButton ( const QIcon & icon, 
                         const QString & text, 
                         QWidget * parent)
  :QPushButton (icon,text,parent),
   parentWidget (parent),
   trackingMouse (false)
{
  Init ();
}

void
MoveButton::Init ()
{
  QString buttonStyle (
                   " QPushButton { "
                      " border: 2px dashed #8f8f8f; "
                       "   border-radius: 12px; "
                        "  background-color: rgba(100, 100, 200, 10); "
                           "}" 
                   " QPushButton:pressed { "
                      " border: 1px solid #8f8f8f; "
                       "   border-radius: 12px; "
                        "  background-color: rgba(200, 200, 200, 0); "
                           "}" 

                      );
  setStyleSheet (buttonStyle);
  connect (this, SIGNAL (pressed()), this, SLOT (Pressed()));
  connect (this, SIGNAL (released()), this, SLOT (Released()));
}

void
MoveButton::Pressed ()
{
qDebug () << " pressed " << objectName();
  ChangeTracking (true);
}

void
MoveButton::Released ()
{
qDebug () << " release " << objectName();
  ChangeTracking (false);
}

bool
MoveButton::Tracking ()
{
  return trackingMouse;
}

void
MoveButton::leaveEvent (QEvent * event)
{
qDebug () << "leaveEvent " << event << objectName();
  ChangeTracking (false);
  QPushButton::leaveEvent (event);
}

void
MoveButton::ChangeTracking (bool doTrack)
{
  trackingMouse = doTrack;
  setMouseTracking (doTrack);
  emit Track (trackingMouse);
  qDebug () << objectName() << hasMouseTracking();
}

void
MoveButton::enterEvent (QEvent * event)
{
  emit Track (trackingMouse);
  QPushButton::enterEvent (event);
}

void
MoveButton::paintEvent (QPaintEvent * event)
{
  QSize canvasSize = size();
  int w = canvasSize.width();
  int h = canvasSize.height();
  const int boxSize (8);
  QPainter painter;
  painter.begin (this);
  painter.setPen (QColor(200,0,100,200));
  painter.translate (w/2,h/2);
  painter.drawLine (-30,0, -boxSize,0);
  painter.drawLine (boxSize,0, 30,0);
  painter.drawLine (0,-30,0,-boxSize);
  painter.drawLine (0,boxSize,0,30);
  painter.drawRect (-boxSize, -boxSize,2*boxSize,2*boxSize);
  painter.end ();
  QPushButton::paintEvent (event);
}

void
MoveButton::mouseMoveEvent (QMouseEvent * event)
{
  static int count (0);
  if (trackingMouse) {
    qDebug () << objectName () << " following mouse " << event
            <<  count++;
    parentWidget->update ();
  }
}


void
MoveButton::CenterOn (QPoint center)
{
  CenterOn (center.x(), center.y());
}

void
MoveButton::CenterOn (int x, int y)
{
  int w = size().width();
  int h = size().height();
  move (x-w/2, y-h/2);
}

QPoint
MoveButton::Center ()
{
  int w = size().width();
  int h = size().height();
  return QPoint (pos().x()+w/2,pos().y()+h/2);
}

QVector2D
MoveButton::Direction ()
{
  QVector2D cur = QVector2D (parentWidget->mapFromGlobal (QCursor::pos()));
  QVector2D home = QVector2D (Center());
  QVector2D dir = cur - home;
  return dir.normalized();
}

} // namespace
