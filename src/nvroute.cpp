#include "nvroute.h"

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

#include <QApplication>
#include "deliberate.h"
#include "version.h"
#include "helpview.h"
#include "navi-global.h"
#include <QSize>
#include <QDebug>
#include <QMessageBox>
#include <QTimer>
#include <QTime>
#include <QCursor>
#include <QFileDialog>
#include <QFile>
#include <QTreeWidgetItem>
#include <QTreeWidget>


using namespace deliberate;

namespace navi
{

NvRoute::NvRoute (QWidget *parent)
  :QMainWindow (parent),
   initDone (false),
   app (0),
   configEdit (this),
   helpView (0),
   runAgain (false),
   db (this)
{
  mainUi.setupUi (this);
  mainUi.actionRestart->setEnabled (false);
  helpView = new HelpView (this);
  Connect ();
}

void
NvRoute::Init (QApplication &ap)
{
  app = &ap;
  connect (app, SIGNAL (lastWindowClosed()), this, SLOT (Exiting()));
  Settings().sync();
  db.Start ();
  initDone = true;
}

bool
NvRoute::Again ()
{
  bool again = runAgain;
  runAgain = false;
  return again;
}

bool
NvRoute::Run ()
{
  runAgain = false;
  if (!initDone) {
    Quit ();
    return false;
  }
  qDebug () << " Start Collect";
  QSize defaultSize = size();
  QSize newsize = Settings().value ("sizes/main", defaultSize).toSize();
  resize (newsize);
  Settings().setValue ("sizes/main",newsize);
  show ();
  SetDefaults ();
  return true;
}

void
NvRoute::SetDefaults ()
{
  double defLat (0.0);
  double defLon (0.0);
  defLat = Settings().value ("defaults/lat",defLat).toDouble();
  Settings().setValue ("defaults/lat",defLat);
  defLon = Settings().value ("defaults/lon",defLon).toDouble();
  Settings().setValue ("defaults/lon",defLon);
  Settings().sync();
  mainUi.latValue->setValue (defLat);
  mainUi.lonValue->setValue (defLon);
}

void
NvRoute::Connect ()
{
  connect (mainUi.actionQuit, SIGNAL (triggered()), 
           this, SLOT (Quit()));
  connect (mainUi.actionSettings, SIGNAL (triggered()),
           this, SLOT (EditSettings()));
  connect (mainUi.actionAbout, SIGNAL (triggered()),
           this, SLOT (About ()));
  connect (mainUi.actionLicense, SIGNAL (triggered()),
           this, SLOT (License ()));
  connect (mainUi.actionRestart, SIGNAL (triggered()),
           this, SLOT (Restart ()));
  connect (mainUi.findButton, SIGNAL (clicked()),
           this, SLOT (FindButton ()));
}

void
NvRoute::Restart ()
{
  qDebug () << " restart called ";
  runAgain = true;
  Quit ();
}


void
NvRoute::Quit ()
{
  CloseCleanup ();
  if (app) {
    app->quit();
  }
}

void
NvRoute::closeEvent (QCloseEvent *event)
{
  Q_UNUSED (event)
  CloseCleanup ();
}

void
NvRoute::CloseCleanup ()
{
  QSize currentSize = size();
  Settings().setValue ("sizes/main",currentSize);
  Settings().sync();
}

void
NvRoute::EditSettings ()
{
  configEdit.Exec ();
  SetSettings ();
}

void
NvRoute::SetSettings ()
{
  Settings().sync ();
}

void
NvRoute::About ()
{
  QString version (deliberate::ProgramVersion::Version());
  QStringList messages;
  messages.append (version);
  messages.append (configMessages);

  QMessageBox  box;
  box.setText (version);
  box.setDetailedText (messages.join ("\n"));
  QTimer::singleShot (30000, &box, SLOT (accept()));
  box.exec ();
}

void
NvRoute::Exiting ()
{
  QSize currentSize = size();
  Settings().setValue ("sizes/main",currentSize);
  Settings().sync();
}

void
NvRoute::License ()
{
  if (helpView) {
    helpView->Show ("qrc:/help/LICENSE.txt");
  }
}

void
NvRoute::FindButton ()
{
  double lat = mainUi.latValue->value();
  double lon = mainUi.lonValue->value();
  Settings().setValue ("defaults/lat",lat);
  Settings().setValue ("defaults/lon",lon);
  quint64 parcel = Parcel::Index (lat,lon);
  mainUi.logDisplay->append (QString ("lat %1 lon %2 is parcel %3 (0x%4)")
                             .arg (lat)
                             .arg (lon)
                             .arg (parcel)
                             .arg (QString::number(parcel,16)));
  QStringList wayList;
  bool ok = db.GetWays (parcel,wayList);
  int nways = wayList.count();
  mainUi.logDisplay->append (QString ("GetWay was %1").arg(ok));
  mainUi.logDisplay->append (QString ("  have %1 ways:").arg(nways));
  for (int w=0; w<nways; w++) {
    QString wayId = wayList.at(w);
    mainUi.logDisplay->append (QString ("  Way %1")
                              .arg(wayId));
    ListWayDetails (wayId);
  }
  mainUi.logDisplay->append ("---------");
}

void
NvRoute::ListWayDetails (const QString & wayId)
{
  QString name ("not named");
  bool hasName = db.GetWayTag (wayId, "name",name);
  if (!hasName) {
    return;
  }
  QTreeWidget *tree = mainUi.wayTree;
  QStringList labels;
  labels << wayId;
  labels << name;
  QString highwayType ("?");
  bool isHighway = db.GetWayTag (wayId, "highway", highwayType);
  labels << highwayType;
  
  QTreeWidgetItem *wayItem = new QTreeWidgetItem (tree, labels);
  QStringList nodeList;
  bool hasNodes = db.GetWayNodes (wayId, nodeList);
  if (hasNodes) {
    QList <QTreeWidgetItem*> itemList;
    QTreeWidgetItem *nodeItem;
    for (int n=0; n<nodeList.count(); n++) {
      nodeItem = new QTreeWidgetItem;
      QString nodeId = nodeList.at (n);
      nodeItem->setText (0,nodeId);
      double lat, lon;
      bool haveCoord = db.GetNode (nodeId, lat, lon);
      if (haveCoord) {
        nodeItem->setText (1,QString::number (lat));
        nodeItem->setText (2,QString::number (lon));
      }
      itemList.append (nodeItem);
    } 
    wayItem->addChildren (itemList);
  }
}


} // namespace

