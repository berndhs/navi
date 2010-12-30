#include "asroute.h"


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
#include "version.h"

#include <QMessageBox>
#include <QTimer>
#include <QThread>
#include <QDebug>

using namespace deliberate;

namespace navi
{
AsRoute::AsRoute (QWidget *parent)
  :QMainWindow (parent),
   app (0),
   db (this),
   configEdit (this),
   helpView (0),
   cellMenu (0)
{
  mainUi.setupUi (this);
  mainUi.actionRestart->setEnabled (false);
  helpView = new HelpView (this);
  cellMenu = new RouteCellMenu (this);
  cellTypeName[Cell_NoType] = "NoType";
  cellTypeName[Cell_Node] = "Node";
  cellTypeName[Cell_Way] = "Way";
  cellTypeName[Cell_Relation] = "Relation";
  cellTypeName[Cell_Tag] = "Tag";
  cellTypeName[Cell_LatLon] = "LatLon";
  cellTypeName[Cell_Header] = "Header";
  cellTypeName[Cell_Bad] = "Bad";
  Connect ();
}

void
AsRoute::Init (QApplication & qapp)
{
  app = &qapp;
  QCoreApplication::setAttribute (Qt::AA_DontShowIconsInMenus, false);
  Settings().sync();
  db.Start ();
}

void
AsRoute::Run ()
{
  show ();
  qDebug () << " Start AsRoute";
  QSize defaultSize = size();
  QSize newsize = Settings().value ("sizes/main", defaultSize).toSize();
  resize (newsize);
  Settings().setValue ("sizes/main",newsize);
  show ();
  SetDefaults ();
}

void
AsRoute::closeEvent (QCloseEvent *event)
{
  Q_UNUSED (event)
  if (app) {
    app->quit();
  }
}


void
AsRoute::SetDefaults ()
{
  double south (0.0);
  double north (0.0);
  double east (0.0);
  double west (0.0);
  south = Settings().value ("defaults/south",south).toDouble();
  Settings().setValue ("defaults/south",south);
  north = Settings().value ("defaults/north",north).toDouble();
  Settings().setValue ("defaults/north",north);
  east = Settings().value ("defaults/east",east).toDouble();
  Settings().setValue ("defaults/east",east);
  west = Settings().value ("defaults/west",west).toDouble();
  Settings().setValue ("defaults/west",west);
  quint64 parcel (0);
  parcel = Settings().value ("defaults/parcel",parcel).toULongLong();
  Settings().setValue ("defaults/parcel",parcel);
  Settings().sync();
  mainUi.southValue->setValue (south);
  mainUi.northValue->setValue (north);
  mainUi.westValue->setValue (west);
  mainUi.eastValue->setValue (east);
  mainUi.parcelEdit->setText (QString::number(parcel));
}

void
AsRoute::Connect ()
{
  connect (mainUi.actionQuit, SIGNAL (triggered()), 
           this, SLOT (Quit()));
  connect (mainUi.actionSettings, SIGNAL (triggered()),
           this, SLOT (EditSettings()));
  connect (mainUi.actionAbout, SIGNAL (triggered()),
           this, SLOT (About ()));
  connect (mainUi.actionLicense, SIGNAL (triggered()),
           this, SLOT (License ()));
  connect (mainUi.latlonButton, SIGNAL (clicked()),
           this, SLOT (LatLonButton ()));
  connect (mainUi.parcelButton, SIGNAL (clicked()),
           this, SLOT (ParcelButton ()));
  connect (mainUi.featureButton, SIGNAL (clicked()),
           this, SLOT (FeatureButton ()));
  connect (mainUi.featureDisplay, SIGNAL (itemDoubleClicked (QTreeWidgetItem*,int)),
           this, SLOT (Picked (QTreeWidgetItem*, int)));

  connect (&db, SIGNAL (HaveRangeNodes (int, const QStringList &)),
           this, SLOT (HandleRangeNodes (int, const QStringList &)));
  connect (&db, SIGNAL (HaveLatLon (int, double, double)),
           this, SLOT (HandleLatLon (int, double, double)));
}

void
AsRoute::AddConfigMessages (const QStringList & argList)
{
  configMessages = argList;
}


void
AsRoute::Quit ()
{
  CloseCleanup ();
  if (app) {
    app->quit();
  }
}

void
AsRoute::CloseCleanup ()
{
  QSize currentSize = size();
  Settings().setValue ("sizes/main",currentSize);
  Settings().sync();
  db.Stop ();
}

void
AsRoute::EditSettings ()
{
  configEdit.Exec ();
  SetSettings ();
}

void
AsRoute::SetSettings ()
{
  Settings().sync ();
}

void
AsRoute::About ()
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
AsRoute::Exiting ()
{
  QSize currentSize = size();
  Settings().setValue ("sizes/main",currentSize);
  Settings().sync();
}

void
AsRoute::License ()
{
  if (helpView) {
    helpView->Show ("qrc:/help/LICENSE.txt");
  }
}

void
AsRoute::FeatureButton ()
{
  QMessageBox::information (this, QString ("Info"), 
                     QString ("Feature Button clicked"));
}

void
AsRoute::LatLonButton ()
{
qDebug () << "LatLonBUtton in thread " << QThread::currentThread();
  double south = mainUi.southValue->value();
  double north = mainUi.northValue->value();
  double west = mainUi.westValue->value();
  double east = mainUi.eastValue->value();
  Settings().setValue ("defaults/south",south);
  Settings().setValue ("defaults/north",north);
  Settings().setValue ("defaults/east",east);
  Settings().setValue ("defaults/west",west);
  Settings().sync();
  nodeSet.clear ();
  db.AskRangeNodes (south,west, north,east);
}

void
AsRoute::ParcelButton ()
{
  QMessageBox::information (this, QString ("Info"), 
                      QString ("Parcel Button clicked"));
}

void
AsRoute::HandleRangeNodes (int reqId, const QStringList & nodes)
{
  for (int n=0;n<nodes.count();n++) {
    nodeSet.insert (nodes.at(n));
  }
  dbRequests.remove (reqId);
qDebug () << " list count " << nodes.count() << " set count " << nodeSet.count();
  ListNodes();
}

void
AsRoute::HandleLatLon (int reqId, double lat, double lon)
{
  if (dbRequests.contains (reqId)) {
    QTreeWidgetItem *item = dbRequests[reqId].destItem;
    if (item) {
      item->setText (1,QString::number (lat));
      item->setText (2,QString::number (lon));
    }
    dbRequests.remove (reqId);
  }
}

void
AsRoute::ListNodes ()
{
qDebug () << "ListNodes in thread " << QThread::currentThread();
  QSet<QString>::iterator nit;
  QTreeWidgetItem * listItem = new QTreeWidgetItem (Cell_Header);
  listItem->setText (0,QString ("found %1 Nodes")
                        .arg (nodeSet.count()));
  QList <QTreeWidgetItem*> nodeList;
int row(0);
  for (nit=nodeSet.begin(); nit!= nodeSet.end(); nit++) {
    QTreeWidgetItem *nodeItem = new QTreeWidgetItem (Cell_Node);
    nodeItem->setText (0,QString("node %1").arg(*nit));
row++;
    if (row < 3000) {
      AskNodeDetails (nodeItem, *nit);
    }
    nodeList.append (nodeItem);
  }
  listItem->addChildren (nodeList);
  mainUi.featureDisplay->addTopLevelItem (listItem);
}

void
AsRoute::AskNodeDetails (QTreeWidgetItem * item, const QString & nodeId)
{
  ResponseStruct resp;
  resp.destItem = item;
  resp.type = Dest_LatLon;
  int reqId = db.AskLatLon (nodeId);
  dbRequests[reqId] = resp;
}

} // namespace
