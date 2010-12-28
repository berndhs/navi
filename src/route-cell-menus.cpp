
#include "route-cell-menus.h"

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

#include <QMenu>
#include <QDesktopServices>
#include <QApplication>
#include <QClipboard>
#include <QUrl>

namespace navi
{

RouteCellMenu::RouteCellMenu (QWidget *parent)
  :QObject (parent),
   parentWidget (parent)
{
  copyAction = new QAction (tr("Copy Text"),this);
  copyAction->setIcon(QIcon(":/copy.png"));
  mailAction = new QAction (tr("Mail Text"),this);
  mailAction->setIcon(QIcon(":/mail.png"));
}

QAction *
RouteCellMenu::CellMenu (QTreeWidgetItem *item,
                   int column,
                   const QList<QAction *>  extraActions)
{
  if (item == 0) {
    return 0;
  }
  QMenu menu (parentWidget);
  menu.addAction (copyAction);
  menu.addAction (mailAction);
  if (extraActions.size() > 0) {
    menu.addSeparator ();
  }
  for (int a=0; a < extraActions.size(); a++) {
    menu.addAction (extraActions.at (a));
  }
  
  QAction * select = menu.exec (QCursor::pos());
  if (select == copyAction) {
    QClipboard *clip = QApplication::clipboard ();
    if (clip) {
      clip->setText (item->text(column));  
    }
    return 0;
  } else if (select == mailAction) {
    QStringList mailBodytotal;
    QString mailBody = item->text(column);
    mailBodytotal << mailBody;
    QString urltext = tr("mailto:?subject=&body=%1")
                      .arg (mailBodytotal.join("\r\n"));
    QDesktopServices::openUrl (QUrl(urltext));
    return 0;
  } else {
    return select;
  }
}

} // namespace
