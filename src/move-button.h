#ifndef NAVI_MOVE_BUTTON_H
#define NAVI_MOVE_BUTTON_H



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



#include <QPushButton>
#include <QPoint>
#include <QEvent>
#include <QPaintEvent>
#include <QMouseEvent>

#include <QVector2D>

namespace navi
{
class MoveButton : public QPushButton
{
Q_OBJECT

public:

  MoveButton ( QWidget * parent = 0 );
  MoveButton ( const QString & text, QWidget * parent = 0 );
  MoveButton ( const QIcon & icon, const QString & text, QWidget * parent = 0 );

  bool Tracking ();

  void CenterOn (QPoint center);
  void CenterOn (int x, int y);
  QPoint Center ();
  QVector2D Direction ();

public slots:

  void Pressed ();
  void Released ();

protected:

  void leaveEvent (QEvent * event);
  void enterEvent (QEvent * event);
  void paintEvent (QPaintEvent * event);
  void mouseMoveEvent (QMouseEvent * event);

signals:

  void Track (bool track);

private:

  void Init ();
  void ChangeTracking (bool doTrack);

  QWidget *parentWidget;
  bool trackingMouse;

};

} // namespace

#endif