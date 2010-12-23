#include "collect.h"

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
#include <QNetworkRequest>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNode>
#include <QDomNodeList>
#include <QFileDialog>
#include <QFile>
#include <QTime>


using namespace deliberate;

namespace navi
{

Collect::Collect (QWidget *parent)
  :QMainWindow (parent),
   initDone (false),
   app (0),
   configEdit (this),
   helpView (0),
   runAgain (false),
   db (this),
   network (this),
   latStep (1.0/60.0),   // 1 arc minute
   lonStep (1.0/60.0),    // 1 arc minute
   autoGet (false)
{
  mClock.start ();
  mainUi.setupUi (this);
  mainUi.actionRestart->setEnabled (false);
  helpView = new HelpView (this);
  Connect ();
}

void
Collect::Init (QApplication &ap)
{
  app = &ap;
  connect (app, SIGNAL (lastWindowClosed()), this, SLOT (Exiting()));
  Settings().sync();
  db.Start ();
  initDone = true;
}

bool
Collect::Again ()
{
  bool again = runAgain;
  runAgain = false;
  return again;
}

bool
Collect::Run ()
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
Collect::SetDefaults ()
{
  QString server ("api.openstreetmap.org");
  server = Settings().value ("defaults/server",server).toString();
  Settings ().setValue ("defaults/server",server);
  Settings().sync();
  mainUi.serverEdit->setText (server);
}

void
Collect::Connect ()
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
  connect (mainUi.readButton, SIGNAL (clicked()),
           this, SLOT (ReadButton ()));
  connect (mainUi.saveButton, SIGNAL (clicked()),
           this, SLOT (SaveResponse()));
  connect (mainUi.xmlReadButton, SIGNAL (clicked()),
           this, SLOT (ReadXML ()));
  connect (mainUi.saveSqlButton, SIGNAL (clicked()),
           this, SLOT (SaveSql ()));

  connect (&network, SIGNAL (finished (QNetworkReply*)),
           this, SLOT (HandleReply (QNetworkReply*)));
}

void
Collect::Restart ()
{
  qDebug () << " restart called ";
  runAgain = true;
  Quit ();
}


void
Collect::Quit ()
{
  CloseCleanup ();
  if (app) {
    app->quit();
  }
}

void
Collect::closeEvent (QCloseEvent *event)
{
  Q_UNUSED (event)
  CloseCleanup ();
}

void
Collect::CloseCleanup ()
{
  QSize currentSize = size();
  Settings().setValue ("sizes/main",currentSize);
  Settings().sync();
}

void
Collect::EditSettings ()
{
  configEdit.Exec ();
  SetSettings ();
}

void
Collect::SetSettings ()
{
  Settings().sync ();
}

void
Collect::About ()
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
Collect::Exiting ()
{
  QSize currentSize = size();
  Settings().setValue ("sizes/main",currentSize);
  Settings().sync();
}

void
Collect::License ()
{
  if (helpView) {
    helpView->Show ("qrc:/help/LICENSE.txt");
  }
}

void
Collect::ReadButton ()
{
  southEnd = mainUi.southValue->value();
  minSouth = southEnd;
  northEnd = mainUi.northValue->value();
  eastEnd = mainUi.eastValue->value ();
  westEnd = mainUi.westValue->value ();
  minWest = westEnd;
  useNetwork = true;
  autoGet = mainUi.autoGet->isChecked();
qDebug () << " ReadButton autoGet " << autoGet;
  SendRequest (westEnd, westEnd + lonStep,
               southEnd, southEnd + latStep);
}

void
Collect::SendNext ()
{
qDebug () << " SendNext autoGet " << autoGet;
  if (!autoGet) {
    return;
  }
  southEnd += latStep;
  if (southEnd > northEnd) {
    southEnd = minSouth;
    westEnd += lonStep;
  }
  if (westEnd < eastEnd || southEnd < northEnd) {
    SendRequest (westEnd, westEnd + lonStep,
                 southEnd, southEnd + latStep);
  } else {
    LogStatus  (QString("ALL DONE with final request %1")
                                       .arg(lastUrl));
    ShowProgress ();
  }
}

void
Collect::SendRequest (double west, double east, double south, double north)
{
  QUrl url;
  url.setScheme ("http");
  url.setHost (mainUi.serverEdit->text());
  url.setPath ("api/0.6/map");
  url.addQueryItem (QString ("bbox"),QString ("%1,%2,%3,%4")
                                      .arg (west).arg (south)
                                      .arg (east).arg (north));
  
  QNetworkRequest  req (url);
  reply = network.get (req);
  LogStatus  ("---------------------------------");
  LogStatus  (QString ("requested: %1")
                               .arg (req.url().toString()));
  lastUrl = req.url().toString();
  LogStatus  (":");
}

void
Collect::HandleReply (QNetworkReply * reply)
{
  responseBytes = reply->readAll ();
  LogStatus  ("vvvvvvvvvvvv");
  LogStatus  ("Received reply to ");
  LogStatus  (reply->url().toString());
  ProcessData (responseBytes);
  if (autoGet && useNetwork) {
    QTimer::singleShot (75,this, SLOT(SaveSql ()));
  }
  LogStatus  ("^^^^^^^^^^");
}

void
Collect::ProcessData (QByteArray & data)
{
  QDomDocument replyDoc;
  nodeMap.clear ();
  wayNodes.clear ();
  wayAttrMap.clear ();
  nodeAttrMap.clear ();
  replyDoc.setContent (data);
  ProcessNodes (replyDoc);
  ProcessWays (replyDoc);
  ProcessRelations (replyDoc);
}

void
Collect::ProcessNodes (QDomDocument & doc)
{
  QDomNodeList  nodes = doc.elementsByTagName ("node");
  for (int i=0; i<nodes.count(); i++) {
    QDomNode node = nodes.at(i);
    if (node.isElement ()) {
      QDomElement elt = node.toElement();
      QString id = elt.attribute ("id");
      double dlat = elt.attribute ("lat").toDouble();
      double dlon = elt.attribute ("lon").toDouble();
      nodeMap[id] = NaviNode (id,dlat,dlon);
      QDomNodeList kids = node.childNodes ();
      AttrList attrList;
      for (int k=0;k<kids.count();k++) {
        QDomNode kid = kids.item(k);
        if (kid.isElement()) {
          QDomElement kidElt = kid.toElement();
          QString tagName = kidElt.tagName();
          if (tagName == "tag") {
            QString key = kidElt.attribute("k");
            QString val = kidElt.attribute("v");
            AttrType nodeAttr (key,val);
            attrList.append (nodeAttr);
          }
        }
      }
      nodeAttrMap [id] = attrList;
    } else {
      LogStatus  ("Way Node not an Element");
    }
  }
}

void
Collect::ProcessWays (QDomDocument & doc)
{
  QDomNodeList  ways = doc.elementsByTagName ("way");
  for (int i=0; i< ways.count(); i++) {
    ProcessWay (ways.item(i));
  }
}

void
Collect::ProcessRelations (QDomDocument & doc)
{
  QDomNodeList  relations = doc.elementsByTagName ("relation");
  for (int i=0;i<relations.count(); i++) {
    ProcessRelation (relations.at(i));
  }
}


void
Collect::ProcessWay (const QDomNode & node)
{
  QString id;
  if (node.isElement ()) {
    QDomElement elt = node.toElement();
    id = elt.attribute ("id");
  } else {
    LogStatus  ("Way Node not an Element");
    return;
  }
  QDomNodeList kids = node.childNodes ();
  QStringList nodeIdList;
  AttrList    attrList;
  for (int k=0; k<kids.count(); k++) {
    QDomNode kid = kids.item(k);
    if (kid.isElement()) {
      QDomElement kidElt = kid.toElement();
      QString tagName = kidElt.tagName();
      if (tagName == "tag") {
        QString key = kidElt.attribute("k");
        QString val = kidElt.attribute("v");
        AttrType wayAttr (key,val);
        attrList.append (wayAttr);
      } else if (tagName == "nd") {
        QString nodeId = kidElt.attribute ("ref");
        nodeIdList.append (nodeId);
      }
    }
  }
  wayNodes [id] = nodeIdList;
  wayAttrMap [id] = attrList;
}

void
Collect::ProcessRelation (const QDomNode & node)
{
  QString id;
  if (node.isElement ()) {
    QDomElement elt = node.toElement();
    id = elt.attribute ("id");
  } else {
    LogStatus  ("Relation Node not an Element");
    return;
  }
  AttrList  memberList;
  AttrList  tagList;
  QDomNodeList kids = node.childNodes ();
  for (int k=0; k<kids.count(); k++) {
    QDomNode kid = kids.at(k);
    if (!kid.isElement()) {
      continue;
    }
    QDomElement kidElt = kid.toElement ();
    QString tagName = kidElt.tagName();
    if (tagName == "member") {
      QString type = kidElt.attribute ("type");
      QString ref = kidElt.attribute ("ref");
      AttrType member (type,ref);
      memberList.append (member);
    } else if (tagName == "tag") {
      QString key = kidElt.attribute ("k");
      QString value = kidElt.attribute ("v");
      AttrType tag (key,value);
      tagList.append (tag);
    } else {
      LogStatus  (QString("unknown relation element %1")
                                 .arg (tagName));
    }
  }
  relationAttrMap [id] = tagList;
  relationMembers [id] = memberList;
}

void
Collect::SaveResponse ()
{
  QString filename = QFileDialog::getSaveFileName (this, "Save Response XML");
  if (filename.length() < 1) {
    return;
  }
  QFile file (filename);
  file.open (QFile::WriteOnly);
  file.write (responseBytes);
  file.close ();
}

void
Collect::ReadXML ()
{
  useNetwork = false;
  QString filename = QFileDialog::getOpenFileName (this, "Read XML File");
  if (filename.length() < 1) {
    return;
  }
  QFile file (filename);
  bool ok = file.open (QFile::ReadOnly);
  if (!ok) {
    return;
  }
  responseBytes.clear ();
  responseBytes = file.readAll();
  file.close ();
  if (responseBytes.size() > 0) {
    ProcessData (responseBytes);
  }
}

void
Collect::SaveSql ()
{
  saveStage = Stage_Nodes;
  QTimer::singleShot (100, this, SLOT (SaveSequence()));
}

void
Collect::ContinueSequence ()
{
  saveStage = Save_Stage (((int) saveStage) + 1);
  if (saveStage <= Stage_None || saveStage >= Stage_Done) {
    saveStage = Stage_None;
  } else {
    QTimer::singleShot (100, this, SLOT (SaveSequence()));
  }
}

void
Collect::LogStatus (const QString & msg)
{
  mainUi.logDisplay->append (QString ("%1 - %2")
                             .arg (mClock.elapsed()/1000.0, 10, 'f')
                             .arg (msg));
}

void
Collect::SaveSequence()
{
  LogStatus (QString ("Save Sequence stage %1").arg (saveStage));
  switch (saveStage) {
  case Stage_Nodes:
    LogStatus ("%1 start saving nodes");
    QTimer::singleShot (100, this, SLOT (SaveNodesSql ()));
    break;
  case Stage_Ways:
    LogStatus ("start saving ways");
    QTimer::singleShot (100, this, SLOT (SaveWaysSql ()));
    break;
  case Stage_Relations:
    LogStatus  ("start saving relations");
    QTimer::singleShot (100, this, SLOT (SaveRelationsSql ()));
    break;
  case Stage_Final:
    LogStatus (QString ("done with %1").arg(lastUrl));
    ShowProgress ();
    saveStage = Stage_Done;
    if (autoGet && useNetwork) {
      QTimer::singleShot (100, this, SLOT (SendNext()));
    }
  default:
    break;
  }
}

void
Collect::ShowProgress ()
{
  int percentLat = qRound (((southEnd - minSouth) 
                           / (northEnd - minSouth)) * 100.0);
  int percentLon = qRound (((westEnd - minWest)
                           / (eastEnd - minWest)) * 100.0);
  mainUi.latProgress->setValue (percentLat);
  mainUi.lonProgress->setValue (percentLon);
}

void
Collect::SaveNodesSql ()
{
  int saved (0);
  db.StartTransaction ();
  QTime clock;
  clock.start ();
  NodeMapType::iterator nit;
  for (nit=nodeMap.begin(); nit!= nodeMap.end(); nit++) {
    NaviNode node = *nit;
    db.WriteNode (node.Id(), node.Lat(), node.Lon());
    db.WriteNodeParcel (node.Id(), Parcel::Index (node.Lat(),node.Lon()));
    saved++;
  }
  QMap <QString, AttrList>::iterator mit;
  for (mit=nodeAttrMap.begin(); mit!= nodeAttrMap.end(); mit++) {
    QString nodeId = mit.key();
    int count = mit->count();
    for (int a=0; a<count; a++) {
      AttrType attr = mit->at(a);
      db.WriteNodeTag (nodeId, attr.first, attr.second);
    }
  }
  db.CommitTransaction ();
  int msecs = clock.elapsed();
  LogStatus  (QString ("Saved %1 nodes in %2 msecs")
                              .arg(saved)
                              .arg (msecs));
  ContinueSequence ();
}

void
Collect::SaveWaysSql ()
{
  int savedid (0);
  int savedtag (0);
  int savednode (0);
  QTime clock;
  clock.start ();
  db.StartTransaction ();
  QMap<QString, QStringList>::iterator wit;
  for (wit=wayNodes.begin(); wit!=wayNodes.end(); wit++) {
    QString wayId = wit.key();
    db.WriteWay (wayId);
    savedid++;
    BuildWayParcels (wayId, *wit);  
    for (int n=0; n<wit->count(); n++) {
      db.WriteWayNode (wayId, wit->at(n));
      savednode++;
    }
  }
  QMap<QString, AttrList>::iterator ait;
  for (ait=wayAttrMap.begin(); ait!=wayAttrMap.end (); ait++) {
    QString wayId = ait.key();
    int count = ait->count();
    for (int i=0; i<count; i++) {
      AttrType  attr = ait->at(i);
      db.WriteWayTag (wayId, attr.first, attr.second);
      savedtag++;
    }
  }
  db.CommitTransaction ();
  int msecs = clock.elapsed ();
  LogStatus  (QString ("wrote %1 ways "
                                      "%2 tags %3 nodes in %4 msecs")
                            .arg (savedid)
                            .arg (savedtag)
                            .arg (savednode)
                            .arg (msecs));
  ContinueSequence ();
}

void
Collect::SaveRelationsSql ()
{
  QMap<QString, AttrList>::iterator ait;
  int savedIds (0);
  int savedTags (0);
  int savedMems (0);
  QTime clock;
  clock.start ();
  db.StartTransaction ();
  for (ait=relationAttrMap.begin(); ait!=relationAttrMap.end (); ait++) {
    QString relId = ait.key();
    db.WriteRelation (relId);
    savedIds++;
    for (int a=0; a<ait->count(); a++) {
      AttrType attr = ait->at(a);
      db.WriteRelationTag (relId, attr.first, attr.second);
      savedTags++;
    }
  }
  for (ait=relationMembers.begin(); ait != relationMembers.end(); ait++) {
    QString relId = ait.key();
    db.WriteRelation (relId);
    for (int a=0; a<ait->count(); a++) {
      AttrType member = ait->at(a);
      QString  type = member.first;
      QString  ref = member.second;
      db.WriteRelationMember (relId, type, ref);
      savedMems++;
    }
  }
  db.CommitTransaction ();
  int msecs = clock.elapsed ();
  LogStatus  (QString ("wrote %1 relations "
                                      "%2 tags %3 members in %4 msecs")
                            .arg (savedIds)
                            .arg (savedTags)
                            .arg (savedMems)
                            .arg (msecs));
  ContinueSequence ();
}

void
Collect::BuildWayParcels (const QString & wayId, 
                       const QStringList & nodeIdList)
{
  QStringList nodeList = wayNodes[wayId];
  //db.StartTransaction ();
  for (int n=0; n<nodeIdList.count (); n++) {
    double lat, lon;
    QString nodeId = nodeIdList.at(n);
    if (nodeMap.contains (nodeId))  {
      NaviNode node = nodeMap[nodeId];
      lat = node.Lat();
      lon = node.Lon();
qDebug () << " way parcel node " << wayId << nodeId;
      db.WriteWayParcel (wayId, Parcel::Index (lat, lon));
    }
  }
  
  //db.CommitTransaction ();
}


Collect::Highway::Highway ()
  :ishighway(false)
{
}

void
Collect::Highway::clear ()
{
  name.clear ();
  kind.clear ();
  ishighway = false;
  attributes.clear();
}


} // namespace

