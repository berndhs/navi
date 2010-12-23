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
  quint64 parcel (0);
  parcel = Settings().value ("defaults/parcel",parcel).toULongLong();
  Settings().setValue ("defaults/parcel",parcel);
  Settings().sync();
  mainUi.latValue->setValue (defLat);
  mainUi.lonValue->setValue (defLon);
  mainUi.parcelEdit->setText (QString::number(parcel));
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
  connect (mainUi.parcelButton, SIGNAL (clicked()),
           this, SLOT (ParcelButton ()));
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
NvRoute::ParcelButton ()
{
  quint64 parcel = mainUi.parcelEdit->text().toLongLong();
qDebug () << "ParcelButton " << parcel;
  Settings().setValue ("defaults/parcel",parcel);
  waySet.clear ();
  nodeSet.clear ();
  relationSet.clear ();
  mainUi.wayTree->clear();
  wayList.clear ();
  FindParcel (parcel);
}

void
NvRoute::FindButton ()
{
  double lat = mainUi.latValue->value();
  double lon = mainUi.lonValue->value();
qDebug () << "FindButton " << lat << lon;
  Settings().setValue ("defaults/lat",lat);
  Settings().setValue ("defaults/lon",lon);
  mainUi.logDisplay->append ("FindButton ++++");
  double offset = 1.0/Parcel::Resolution();
  waySet.clear ();
  nodeSet.clear ();
  relationSet.clear ();
  mainUi.wayTree->clear();
  wayList.clear ();
  for (int i=-1; i<2; i++) {
    for (int j=-1; j<2; j++) {
      double dlat = lat + (double(i) * offset);
      double dlon = lon + (double(j) * offset);
      quint64 parcel = Parcel::Index (dlat,dlon);
      QString msg = QString ("lat %1 lon %2 is parcel %3 (0x%4)\n")
                             .arg (dlat)
                             .arg (dlon)
                             .arg (parcel)
                             .arg (QString::number(parcel,16));
      mainUi.logDisplay->append (msg);
qDebug () << msg;
      FindParcel (parcel);
    }
  }
}

void
NvRoute::FindParcel (quint64 parcel)
{
  mainUi.logDisplay->append (QString ("looking for parcel %1")
                               .arg (parcel));
  parcelIndex = parcel;
qDebug () << "FindParcel want index " << parcelIndex;
  QTimer::singleShot (100,this,SLOT (FindWays()));
  QTimer::singleShot (70,this,SLOT(update()));
}

void
NvRoute::FindWays ()
{
qDebug () << " FindWays";
  bool ok = db.GetWays (parcelIndex,wayList);
  int nways = wayList.count();
qDebug () << " waylist count " << wayList.count();
  waySet = wayList.toSet();
  mainUi.logDisplay->append (QString ("GetWay was %1").arg(ok));
  mainUi.logDisplay->append (QString ("  have %1 ways:").arg(nways));
  for (int w=0; w<nways; w++) {
    QString wayId = wayList.at(w);
    mainUi.logDisplay->append (QString ("  Way %1")
                              .arg(wayId));
    ListWayDetails (wayId);
  }
  ListRelations ();
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
  db.GetWayTag (wayId, "highway", highwayType);
  labels << highwayType;
  QString houseNumber ("no number");
  db.GetWayTag (wayId, "addr:housenumber", houseNumber);
  labels << houseNumber;
  
  QTreeWidgetItem *wayItem = new QTreeWidgetItem (tree, labels);
  QStringList wayNodes;
  bool hasNodes = db.GetWayNodes (wayId, wayNodes);
  if (hasNodes) {
    QList <QTreeWidgetItem*> itemList;
    QTreeWidgetItem *nodeItem;
    for (int n=0; n<wayNodes.count(); n++) {
      nodeItem = new QTreeWidgetItem;
      QString nodeId = wayNodes.at (n);
      ListNodeDetails (nodeItem, nodeId);
      itemList.append (nodeItem);
    } 
    wayItem->addChildren (itemList);
  }
  QStringList wayRelations;
  db.GetRelations (wayId, "way", wayRelations);
  mainUi.logDisplay->append (QString("found %1 way relations")
                             .arg (wayRelations.count()));
  if (wayRelations.count() > 0) {
    QList <QTreeWidgetItem*> itemList;
    QTreeWidgetItem *nodeItem;
    for (int n=0; n<wayRelations.count(); n++) {
      nodeItem = new QTreeWidgetItem;
      nodeItem->setText (0,QString ("relation %1").arg(wayRelations.at(n)));
      itemList.append (nodeItem);
    } 
    wayItem->addChildren (itemList);
  }
  relationSet += wayRelations.toSet();
  nodeSet += wayNodes.toSet();
}

void
NvRoute::ListRelations ()
{
  QSet<QString>::iterator nit;
  for (nit=nodeSet.begin(); nit!= nodeSet.end(); nit++) {
    QStringList nodeRelations;
    db.GetRelations (*nit, "node", nodeRelations);
    nodeSet += nodeRelations.toSet();
    mainUi.logDisplay->append (QString("for Node %1 found %2 relations")
                               .arg (*nit).arg (nodeRelations.count()));
  }
  for (nit=relationSet.begin(); nit!= relationSet.end(); nit++) {
    QTreeWidgetItem *relItem = new QTreeWidgetItem;
    relItem->setText (0,QString("relation %1").arg(*nit));
    mainUi.wayTree->addTopLevelItem (relItem);
  }
}

void
NvRoute::ListNodeDetails (QTreeWidgetItem * nodeItem,
                          const QString & nodeId)
{
  nodeItem->setText (0,nodeId);
  double lat, lon;
  bool haveCoord = db.GetNode (nodeId, lat, lon);
  if (haveCoord) {
    nodeItem->setText (1,QString::number (lat));
    nodeItem->setText (2,QString::number (lon));
  }
  QList <QPair <QString, QString> > tagList;
  db.GetNodeTags (nodeId, tagList);
  QList <QPair <QString, QString> >::iterator lit;
  QList <QTreeWidgetItem*> itemList;
  mainUi.logDisplay->append (QString("Node %1 has %2 tags")
                             .arg (nodeId)
                             .arg (tagList.count()));
  for (lit=tagList.begin(); lit!=tagList.end(); lit++) {
    QTreeWidgetItem * item = new QTreeWidgetItem;
    item->setText (0,lit->first);
    item->setText (1,lit->second);
    itemList.append (item);
  }
  nodeItem->addChildren (itemList);
}


} // namespace

