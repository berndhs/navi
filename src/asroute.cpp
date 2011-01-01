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
#include "sql-run-query.h"

#include <QMessageBox>
#include <QTimer>
#include <QThread>
#include <QDebug>

#include <unistd.h>

using namespace deliberate;

namespace navi
{
AsRoute::AsRoute (QWidget *parent)
  :QMainWindow (parent),
   app (0),
   mapWidget (0),
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
  mapWidget = new MapDisplay (0);
  mapWidget->resize (300,250);
  mapWidget->hide ();
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
  Settings().sync();
  mainUi.southValue->setValue (south);
  mainUi.northValue->setValue (north);
  mainUi.westValue->setValue (west);
  mainUi.eastValue->setValue (east);
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
  connect (mainUi.actionClear, SIGNAL (triggered ()),
           this, SLOT (Clear ()));
  connect (mainUi.latlonButton, SIGNAL (clicked()),
           this, SLOT (LatLonButton ()));
  connect (mainUi.waysButton, SIGNAL (clicked()),
           this, SLOT (FindWays ()));
  connect (mainUi.featureButton, SIGNAL (clicked()),
           this, SLOT (FeatureButton ()));
  connect (mainUi.featureDisplay, SIGNAL (itemDoubleClicked (QTreeWidgetItem*,int)),
           this, SLOT (Picked (QTreeWidgetItem*, int)));

  connect (mainUi.showmapButton, SIGNAL (clicked()),
           this, SLOT (ShowMap ()));
  connect (mainUi.hidemapButton, SIGNAL (clicked()),
           this, SLOT (HideMap ()));
  connect (mainUi.drawButton, SIGNAL (clicked()),
           this, SLOT (DrawMap ()));

  connect (mainUi.queryCountMax, SIGNAL (valueChanged (int)),
           this, SLOT (ChangeMaxCount (int)));

  connect (&db, SIGNAL (HaveRangeNodes (int, const QStringList &)),
           this, SLOT (HandleRangeNodes (int, const QStringList &)));
  connect (&db, SIGNAL (HaveLatLon (int, double, double)),
           this, SLOT (HandleLatLon (int, double, double)));
  connect (&db, SIGNAL (HaveTagList (int, const TagList &)),
           this, SLOT (HandleTagList (int, const TagList &)));
  connect (&db, SIGNAL (HaveWayList (int, const QStringList &)),
           this, SLOT (HandleWayList (int, const QStringList &)));
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
AsRoute::Clear ()
{
  mainUi.featureDisplay->clear();
  mainUi.logDisplay->clear();
  nodeSet.clear ();
  requestInDB.clear ();
  requestToSend.clear ();
}

void
AsRoute::ShowMap ()
{
  mapWidget->show ();
}

void
AsRoute::HideMap ()
{
  mapWidget->hide ();
}

void
AsRoute::DrawMap ()
{
  mapWidget->ClearPoints ();
  QMap <QString, QVector2D>::iterator mit;
  int np(0);
  for (mit=nodeCoords.begin(); mit!=nodeCoords.end(); mit++) {
    mapWidget->AddPoint (mit->toPointF());
    np++;
  }
qDebug () << " DrawMap added " << np << " points";
  mapWidget->repaint ();
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
  numNodeDetails = 0;
  db.AskRangeNodes (south,west, north,east);
  UpdateLoad ();
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
  requestInDB.remove (reqId);
qDebug () << " list count " << nodes.count() << " set count " << nodeSet.count();
  numNodes = nodes.count();
  mainUi.loadBar->setMaximum (numNodes);
  ListNodes();
  UpdateLoad ();
}

void
AsRoute::HandleLatLon (int reqId, double lat, double lon)
{
  if (requestInDB.contains (reqId)) {
    QTreeWidgetItem *item = requestInDB[reqId].destItem;
    requestInDB.remove (reqId);
    if (item) {
      item->setText (1,QString::number (lat));
      item->setText (2,QString::number (lon));
      item->setData (0,int(Data_Lat), QVariant(lat));
      item->setData (0,int(Data_Lon), QVariant(lon));
      nodeCoords [item->data (0, Data_NodeId).toString()] 
           = QVector2D (lon, -lat);
    }
  }
  UpdateLoad ();
}

void
AsRoute::HandleTagList (int reqId, const TagList & tagList)
{
  if (!requestInDB.contains (reqId)) {
qDebug () << " dont know request " << reqId;
    return;
  }
qDebug () << " handle tag response " << reqId;
  QTreeWidgetItem * treeItem = requestInDB[reqId].destItem;
  requestInDB.remove (reqId);
  if (treeItem == 0) {
qDebug () << " no item for reqst " << reqId;
    return;
  }
  for (int t=0; t<tagList.count(); t++) {
    TagItemType tag = tagList.at(t);
    QTreeWidgetItem * tagItem = new QTreeWidgetItem;
    tagItem->setText (0, tag.first);
    tagItem->setText (1, tag.second);
    treeItem->addChild (tagItem);
  }
  numNodeDetails ++;
  mainUi.loadBar->setValue (numNodeDetails);
  UpdateLoad ();
}

void
AsRoute::HandleWayList (int reqId, const QStringList & wayList)
{
  if (!requestInDB.contains (reqId)) {
qDebug () << " unkown request " << reqId;
    return;
  }
  requestInDB.remove (reqId);
  QStringList::const_iterator sit;
  for (sit = wayList.begin(); sit != wayList.end(); sit++) {
    waySet.insert (*sit);
    QTreeWidgetItem * wayItem = new QTreeWidgetItem;
    wayItem->setText (0,QString ("way %1").arg (*sit));
    mainUi.featureDisplay->addTopLevelItem (wayItem);
    // FindWayDetails (wayItem, *sit);
  }
  UpdateLoad ();
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
  numNodeDetails = 0;
  mainUi.loadBar->setValue (numNodeDetails);
  for (nit=nodeSet.begin(); nit!= nodeSet.end(); nit++) {
    QTreeWidgetItem *nodeItem = new QTreeWidgetItem (Cell_Node);
    nodeItem->setText (0,QString("node %1").arg(*nit));
    nodeItem->setData (0, Data_NodeId, *nit);
    QueueAskNodeDetails (nodeItem, *nit);
    nodeList.append (nodeItem);
  }
  listItem->addChildren (nodeList);
  mainUi.featureDisplay->addTopLevelItem (listItem);
  KickRequestQueue ();
}

void
AsRoute::KickRequestQueue ()
{
  QTimer::singleShot (100, this, SLOT (SendSomeRequests()));
}

void
AsRoute::QueueAskNodeDetails (QTreeWidgetItem * item, const QString & id)
{
  RequestStruct llReq;
  llReq.type = Req_LatLon;
  llReq.destItem = item;
  llReq.id = id;
  requestToSend.append (llReq);

  RequestStruct tagReq;
  tagReq.type = Req_NodeTagList;
  tagReq.id = id;
  tagReq.destItem = item;
  requestToSend.append (tagReq);
}

void
AsRoute::SendSomeRequests ()
{
  static const int MaxSend (1*1024);
  static const int MaxPending (2*1024);
  int some (MaxSend);
  while (some > 0 
         && !requestToSend.isEmpty()
         && db.PendingRequestCount() < MaxPending) {
    some--;
    RequestStruct req = requestToSend.takeFirst();
    switch (req.type) {
    case Req_LatLon:
      AskLatLon (req.destItem, req.id);
      break;
    case Req_NodeTagList:
      AskNodeTagList (req.destItem, req.id);
      break;
    default:
      break;
    }
  }
  if (!requestToSend.isEmpty ()) {
    KickRequestQueue();
  }
}

void
AsRoute::AskLatLon (QTreeWidgetItem * item, const QString & nodeId)
{
  ResponseStruct resp;
  resp.destItem = item;
  resp.type = Req_LatLon;
  int reqId = db.AskLatLon (nodeId);
  requestInDB[reqId] = resp;
}

void
AsRoute::AskNodeTagList (QTreeWidgetItem *item, const QString & nodeId)
{
  ResponseStruct resp;
  resp.destItem = item;
  resp.type = Req_NodeTagList;
  int reqId = db.AskNodeTagList (nodeId);
  requestInDB[reqId] = resp;
}

void
AsRoute::FindWays ()
{
  QSet<QString>::iterator  nit;
  for (nit = nodeSet.begin(); nit!=nodeSet.end(); nit++) {
    ResponseStruct resp;
    resp.type = Req_WayList;
    int reqId = db.AskWaysByNode (*nit);
    requestInDB[reqId] = resp;
qDebug () << " find way for node " << *nit;
  }
  KickRequestQueue ();
}

void
AsRoute::UpdateLoad ()
{
  int numQueries = db.PendingRequestCount ();
  int max = mainUi.queryCount->maximum();
  if (max < numQueries) {
    max = numQueries * 2;
    mainUi.queryCountMax->setValue (max);
  }
  mainUi.queryCount->setValue (numQueries);
}

void
AsRoute::ChangeMaxCount (int newmax)
{
  int val = mainUi.queryCount->value ();
  int oldmax = mainUi.queryCount->maximum ();
  if (val <= newmax && newmax != oldmax) {
    mainUi.queryCount->setMaximum (newmax);
    mainUi.queryCount->update();
  }
}

} // namespace
