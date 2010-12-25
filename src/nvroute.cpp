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
#include <QAction>
#include <QMenu>
#include <QClipboard>
#include <QDesktopServices>


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
   db (this),
   findTimer (0)
{
  mainUi.setupUi (this);
  mainUi.actionRestart->setEnabled (false);
  helpView = new HelpView (this);
  findTimer = new QTimer (this);
  Connect ();
}

void
NvRoute::Init (QApplication &ap)
{
  app = &ap;
  QCoreApplication::setAttribute (Qt::AA_DontShowIconsInMenus, false);
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
  connect (findTimer, SIGNAL (timeout()), this, SLOT (FindThings()));
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
  connect (mainUi.featureButton, SIGNAL (clicked()),
           this, SLOT (FeatureButton ()));
  connect (mainUi.featureDisplay, SIGNAL (itemDoubleClicked (QTreeWidgetItem*,int)),
           this, SLOT (Picked (QTreeWidgetItem*, int)));
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
  mainUi.featureDisplay->clear();
  wayList.clear ();
  
  findTimer->start (10000);
  int round (0);
  int extend = mainUi.extendedSize->value();
  for (qint64 i= -extend; i <= extend; i++) {
    for (qint64 j= -extend; j <= extend; j++) {
      qint64 latOffset = i * Q_INT64_C (0x0100000000);
      qint64 lonOffset = j * Q_INT64_C (1);
      FindParcel (parcel + latOffset + lonOffset, round);
      round ++;
    }
  }
  QTimer::singleShot (100,this, SLOT (FindThings()));
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
  mainUi.featureDisplay->clear();
  wayList.clear ();
  findTimer->start (10000);
  int round (0);
  int extend = mainUi.extendedSize->value();
  for (int i= -extend; i<=extend; i++) {
    for (int j= -extend ; j<=extend; j++) {
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
      FindParcel (parcel, round);
      round++;
    }
  }
  QTimer::singleShot (100,this, SLOT (FindThings));
}

void
NvRoute::FeatureButton ()
{
  QString name = mainUi.featureEdit->text ();
  bool regular = mainUi.regularCheck->isChecked();
qDebug () << "Feature Button " << name << regular;
  QStringList idList;
  db.GetByTag (idList, "name", name, "node", regular);
  nodeSet = idList.toSet();
  db.GetByTag (idList, "name", name, "way", regular);
  waySet = idList.toSet();
  db.GetByTag (idList, "name", name, "relation", regular);
  relationSet = idList.toSet ();
  mainUi.featureDisplay->clear();
  ListNodes ();
  ListWays ();
  ListRelations ();
}

void
NvRoute::FindParcel (quint64 parcel, int round)
{
  Q_UNUSED (round)
  mainUi.logDisplay->append (QString ("looking for parcel %1")
                               .arg (parcel));
  indexList.append ( parcel);
qDebug () << "FindParcel want index " << parcelIndex;
}

void
NvRoute::FindThings ()
{
  qDebug () << "FindThings indexList has " 
            << indexList.count() 
            << " entries";
  if (indexList.isEmpty()) {
    findTimer->stop ();
    mainUi.logDisplay->append ("no more parcelIndex on list");
    qDebug () << " stopped findTimer";
    return;
  }
  mainUi.featureDisplay->clear();
  FindWays ();
  QStringList nodeList;
  parcelIndex = indexList.takeFirst();
  qDebug () << " FindThings want index " << parcelIndex;
  db.GetNodes (parcelIndex, nodeList);
  QSet<QString> localNodes = nodeList.toSet();
  nodeSet += localNodes;
  mainUi.logDisplay->append (tr("Number nodes before relations %1")
                             .arg (nodeSet.count()));
  FindRelations ();
  FindNodes ();
  QTreeWidgetItem * nodeListItem = new QTreeWidgetItem;
  QList <QTreeWidgetItem*> itemList;
  QTreeWidgetItem *nodeItem;
  QSet <QString>::iterator nit;
  for (nit=nodeSet.begin(); nit!= nodeSet.end(); nit++) {
    nodeItem = new QTreeWidgetItem;
    QString nodeId = *nit;
    ListNodeDetails (nodeItem, nodeId);
    itemList.append (nodeItem);
  } 
  nodeListItem->addChildren (itemList);
  nodeListItem->setText (0,tr("All Nodes"));
  mainUi.featureDisplay->addTopLevelItem (nodeListItem);
  mainUi.logDisplay->append (tr("Number nodes after relations %1")
                             .arg (nodeSet.count()));
  mainUi.logDisplay->append (tr("number parcels to go %1")
                             .arg (indexList.count()));
}

void
NvRoute::FindWays ()
{
qDebug () << " FindWays";
  bool ok = db.GetWays (parcelIndex,wayList);
  int nways = wayList.count();
qDebug () << " waylist count " << wayList.count();
  waySet += wayList.toSet();
  mainUi.logDisplay->append (QString ("GetWay was %1").arg(ok));
  mainUi.logDisplay->append (QString ("  have %1 ways:").arg(nways));
  QSet<QString>::iterator wit;
  for (wit=waySet.begin(); wit!= waySet.end(); wit++) {
    QString wayId = *wit;
    mainUi.logDisplay->append (QString ("  Way %1")
                              .arg(wayId));
    ListWayDetails (wayId);
  }
  FindRelations ();
  ListNodeRelations ();
  mainUi.logDisplay->append ("---------");
}

void
NvRoute::FindRelations ()
{
  QSet<QString>::iterator  sit;
  for (sit=nodeSet.begin(); sit!= nodeSet.end(); sit++) {
    QStringList relationList;
    db.GetRelations (*sit, "node", relationList);
    relationSet += relationList.toSet();
    qDebug () << QString (" node %1 in %2 relations")
                 .arg (*sit).arg (relationList.count());
  }
}

void
NvRoute::FindNodes ()
{
  QSet<QString>::iterator sit;
  for (sit=relationSet.begin(); sit!=relationSet.end(); sit++) {
    QStringList nodeList;
    db.GetRelationMembers (*sit, "node", nodeList);
    nodeSet += nodeList.toSet();
  }
}

void
NvRoute::ListWayDetails (const QString & wayId)
{
  QString name ("not named");
  bool hasName = db.GetWayTag (wayId, "name",name);
  if (!hasName) {
    return;
  }
  QTreeWidget *tree = mainUi.featureDisplay;
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
NvRoute::ListNodeRelations ()
{
  QSet<QString>::iterator nit;
  for (nit=nodeSet.begin(); nit!= nodeSet.end(); nit++) {
    QStringList nodeRelations;
    db.GetRelations (*nit, "node", nodeRelations);
    nodeSet += nodeRelations.toSet();
    mainUi.logDisplay->append (QString("for Node %1 found %2 relations")
                               .arg (*nit).arg (nodeRelations.count()));
  }
  ListRelations ();
}

void
NvRoute::ListRelations ()
{
  QSet<QString>::iterator nit;
  QTreeWidgetItem * listItem = new QTreeWidgetItem;
  listItem->setText (0,QString ("found %1 Relations")
                        .arg (relationSet.count()));
  QList <QTreeWidgetItem*> relList;
  for (nit=relationSet.begin(); nit!= relationSet.end(); nit++) {
    QTreeWidgetItem *relItem = new QTreeWidgetItem;
    relItem->setText (0,QString("relation %1").arg(*nit));
    ListRelationDetails (relItem, *nit);
    relList.append (relItem);
  }
  listItem->addChildren (relList);
  mainUi.featureDisplay->addTopLevelItem (listItem);
}

void
NvRoute::ListNodes ()
{
  QSet<QString>::iterator nit;
  QTreeWidgetItem * listItem = new QTreeWidgetItem;
  listItem->setText (0,QString ("found %1 Nodes")
                        .arg (nodeSet.count()));
  QList <QTreeWidgetItem*> nodeList;
  for (nit=nodeSet.begin(); nit!= nodeSet.end(); nit++) {
    QTreeWidgetItem *nodeItem = new QTreeWidgetItem;
    nodeItem->setText (0,QString("relation %1").arg(*nit));
    ListNodeDetails (nodeItem, *nit);
    nodeList.append (nodeItem);
  }
  listItem->addChildren (nodeList);
  mainUi.featureDisplay->addTopLevelItem (listItem);
}

void
NvRoute::ListWays ()
{
  QSet<QString>::iterator nit;
  QTreeWidgetItem * listItem = new QTreeWidgetItem;
  listItem->setText (0,QString ("found %1 Ways")
                        .arg (waySet.count()));
  QList <QTreeWidgetItem*> wayList;
  for (nit=waySet.begin(); nit!= waySet.end(); nit++) {
    QTreeWidgetItem *wayItem = new QTreeWidgetItem;
    wayItem->setText (0,QString("way %1").arg(*nit));
    ListWayDetails (wayItem, *nit);
    wayList.append (wayItem);
  }
  listItem->addChildren (wayList);
  mainUi.featureDisplay->addTopLevelItem (listItem);
}

void
NvRoute::ListRelationDetails (QTreeWidgetItem *relItem,
                              const QString & relId)
{
  QList <QPair <QString, QString> > tagList;
  db.GetRelationTags (relId, tagList);
  QList <QTreeWidgetItem*> itemList;
  QList <QPair <QString, QString> >::iterator lit;
  for (lit=tagList.begin(); lit!=tagList.end(); lit++) {
    QTreeWidgetItem * item = new QTreeWidgetItem;
    item->setText (0,lit->first);
    item->setText (1,lit->second);
    itemList.append (item);
  }
  relItem->addChildren (itemList);
}

void
NvRoute::ListWayDetails (QTreeWidgetItem *wayItem,
                              const QString & wayId)
{
  QList <QPair <QString, QString> > tagList;
  db.GetWayTags (wayId, tagList);
  QList <QTreeWidgetItem*> itemList;
  QList <QPair <QString, QString> >::iterator lit;
  for (lit=tagList.begin(); lit!=tagList.end(); lit++) {
    QTreeWidgetItem * item = new QTreeWidgetItem;
    item->setText (0,lit->first);
    item->setText (1,lit->second);
    itemList.append (item);
  }
  wayItem->addChildren (itemList);
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

void
NvRoute::Picked (QTreeWidgetItem *item, int column)
{
  CellMenuTop (item, column);
}

QAction *
NvRoute::CellMenu (const QTreeWidgetItem *item,
                   int column,
                   const QList<QAction *>  extraActions)
{
  if (item == 0) {
    return 0;
  }
  QMenu menu (this);
  QIcon copyIcon (":/copy.png");
  mainUi.parcelButton->setIcon (copyIcon);
  QAction * copyAction = new QAction (tr("Copy Text"),this);
  copyAction->setIcon(copyIcon);
  QAction * mailAction = new QAction (tr("Mail Text"),this);
  mailAction->setIcon(QIcon(":/mail.png"));
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
    QDesktopServices::openUrl (urltext);
    return 0;
  } else {
    return select;
  }
}

void
NvRoute::CellMenuTop (const QTreeWidgetItem * item, int column)
{
  if (item == 0) {
    return;
  }
  QAction *collectAction = new QAction (tr("Collect Related Features")
                                          , this);
  collectAction->setIcon(QIcon(":/polar.png"));
  QList <QAction*> list;
  list.append (collectAction);

  QAction * select = CellMenu (item, column, list);
  bool    mailit (false);
  if (select == collectAction) {
  }
}

} // namespace

