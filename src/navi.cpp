#include "navi.h"

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


using namespace deliberate;

namespace navi
{

Navi::Navi (QWidget *parent)
  :QMainWindow (parent),
   initDone (false),
   app (0),
   configEdit (this),
   helpView (0),
   runAgain (false),
   db (this),
   network (this)
{
  mainUi.setupUi (this);
  mainUi.actionRestart->setEnabled (false);
  helpView = new HelpView (this);
  Connect ();
}

void
Navi::Init (QApplication &ap)
{
  app = &ap;
  connect (app, SIGNAL (lastWindowClosed()), this, SLOT (Exiting()));
  Settings().sync();
  db.Start ();
  initDone = true;
}

bool
Navi::Again ()
{
  bool again = runAgain;
  runAgain = false;
  return again;
}

bool
Navi::Run ()
{
  runAgain = false;
  if (!initDone) {
    Quit ();
    return false;
  }
  qDebug () << " Start Navi";
  QSize defaultSize = size();
  QSize newsize = Settings().value ("sizes/main", defaultSize).toSize();
  resize (newsize);
  Settings().setValue ("sizes/main",newsize);
  show ();
  SetDefaults ();
  return true;
}

void
Navi::SetDefaults ()
{
  QString server ("xapi.openstreetmap.org");
  server = Settings().value ("defaults/server",server).toString();
  Settings ().setValue ("defaults/server",server);
  double lat (41.89);
  lat = Settings().value ("defaults/latitude",lat).toDouble();
  Settings().setValue ("defaults/latitude",lat);
  double lon (12.491);
  lon = Settings().value ("defaults/longitude",lon).toDouble();
  Settings().setValue ("defaults/longitude",lon);
  double lonRange (0.5);
  lonRange = Settings().value ("defaults/longrange",lonRange).toDouble();
  Settings().setValue ("defaults/longrange",lonRange);
  double latRange (0.6);
  latRange = Settings().value ("defaults/latrange",latRange).toDouble();
  Settings().setValue ("defaults/latrange",latRange);
  Settings().sync();
  mainUi.serverEdit->setText (server);
  mainUi.lonValue->setValue (lon);
  mainUi.latValue->setValue (lat);
  mainUi.latRange->setValue (latRange);
  mainUi.lonRange->setValue (lonRange);
}

void
Navi::Connect ()
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
Navi::Restart ()
{
  qDebug () << " restart called ";
  runAgain = true;
  Quit ();
}


void
Navi::Quit ()
{
  CloseCleanup ();
  if (app) {
    app->quit();
  }
}

void
Navi::closeEvent (QCloseEvent *event)
{
  Q_UNUSED (event)
  CloseCleanup ();
}

void
Navi::CloseCleanup ()
{
  QSize currentSize = size();
  Settings().setValue ("sizes/main",currentSize);
  Settings().sync();
}

void
Navi::EditSettings ()
{
  configEdit.Exec ();
  SetSettings ();
}

void
Navi::SetSettings ()
{
  Settings().sync ();
}

void
Navi::About ()
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
Navi::Exiting ()
{
  QSize currentSize = size();
  Settings().setValue ("sizes/main",currentSize);
  Settings().sync();
}

void
Navi::License ()
{
  if (helpView) {
    helpView->Show ("qrc:/help/LICENSE.txt");
  }
}

void
Navi::ReadButton ()
{
  double lat = mainUi.latValue->value ();
  double lon = mainUi.lonValue->value ();
  QUrl url;
  url.setScheme ("http");
  url.setHost (mainUi.serverEdit->text());
  url.setPath ("api/0.6/map");
  double partMinLat = mainUi.latRange->value() / 60.0;
  double partMinLon = mainUi.lonRange->value() / 60.0;
  double left = lon - partMinLon;
  double right = lon + partMinLon;
  double top = lat + partMinLat;
  double bot = lat - partMinLat;
  url.addQueryItem (QString ("bbox"),QString ("%1,%2,%3,%4")
                                      .arg (left).arg (bot)
                                      .arg (right).arg (top));
  
  QNetworkRequest  req (url);
  reply = network.get (req);
  mainUi.logDisplay->append ("---------------------------------");
  mainUi.logDisplay->append (QString ("requested: %1")
                               .arg (req.url().toString()));
  mainUi.logDisplay->append (":");
}

void
Navi::HandleReply (QNetworkReply * reply)
{
  responseBytes = reply->readAll ();
  mainUi.logDisplay->append ("vvvvvvvvvvvv");
  mainUi.logDisplay->append ("Reply to ");
  mainUi.logDisplay->append (reply->url().toString());
 // mainUi.logDisplay->append (QString (data));
  ProcessData (responseBytes);
  mainUi.logDisplay->append ("^^^^^^^^^^");
}

void
Navi::ProcessData (QByteArray & data)
{
  QDomDocument replyDoc;
  replyDoc.setContent (data);
  QDomNodeList  ways = replyDoc.elementsByTagName ("way");
  for (int i=0; i< ways.count(); i++) {
    ShowWay (ways.item(i));
  }
}

void
Navi::ShowWay (const QDomNode & node)
{
  Highway  highway;
  QDomNodeList kids = node.childNodes ();
  QString id;
  if (node.isElement ()) {
    QDomElement elt = node.toElement();
    id = elt.attribute ("id");
  } else {
    mainUi.logDisplay->append ("Way Node not an Element");
  }
  for (int k=0; k<kids.count(); k++) {
    QDomNode kid = kids.item(k);
    if (kid.isElement()) {
      QDomElement kidElt = kid.toElement();
      if (kidElt.tagName() == "tag") {
        QString key = kidElt.attribute("k");
        QString val = kidElt.attribute("v");
        if (key == "highway") {
          highway.kind = val;
          highway.ishighway = true;
        } else if (key == "name") {
          highway.name = val;
        }
        highway.attributes[key] = val;
      } 
    }
  }
  if (highway.ishighway) {
    mainUi.logDisplay->append (tr ("Highway %1 is %2")
                                .arg (highway.name)
                                .arg (highway.kind));
    QMap<QString, QString>::iterator mit;
    for (mit = highway.attributes.begin();
         mit != highway.attributes.end();
         mit++) {
      mainUi.logDisplay->append (tr ("  attr %1 = \"%2\"")
                                  .arg (mit.key())
                                  .arg (mit.value ()));
    }
  } else {
    mainUi.logDisplay->append (tr("ignore non-highway Way %1").arg(id));
  }
}

void
Navi::SaveResponse ()
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
Navi::ReadXML ()
{
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
Navi::SaveSql ()
{
  QDomDocument replyDoc;
  replyDoc.setContent (responseBytes);
  SaveNodesSql (replyDoc);
  SaveWaysSql (replyDoc);
}

void
Navi::SaveNodesSql (QDomDocument & doc)
{
  QDomNodeList  nodes = doc.elementsByTagName ("node");
  int saved (0);
  db.StartTransaction ();
  QTime clock;
  clock.start ();
  for (int i=0; i< nodes.count(); i++) {
    QDomNode node = nodes.at(i);
    if (node.isElement()) {
      QDomElement elt = node.toElement();
      QString id = elt.attribute ("id");
      QString slat = elt.attribute ("lat");
      QString slon = elt.attribute ("lon");
      if (id.length() > 0 && slat.length() > 0 && slon.length() > 0) {
        double dlat = slat.toDouble ();
        double dlon = slon.toDouble ();
        db.WriteNode (id, dlat, dlon);
        saved++;
      }
    }
  }
  db.CommitTransaction ();
  int msecs = clock.elapsed();
  mainUi.logDisplay->append (QString ("Saved %1 nodes in %2 msecs")
                              .arg(saved)
                              .arg (msecs));
}

void
Navi::SaveWaysSql (QDomDocument & doc)
{
  QDomNodeList  ways = doc.elementsByTagName ("way");
  int savedid (0);
  int savedtag (0);
  int savednode (0);
  db.StartTransaction ();
  QTime clock;
  clock.start ();
  for (int i=0; i< ways.count(); i++) {
    QString wayid;
    QDomNode way = ways.at(i);
    if (way.isElement ()) {
      QDomElement elt = way.toElement();
      wayid = elt.attribute ("id");
      db.WriteWay (wayid);
      savedid ++;
    } else {
      continue;
    }
    QDomNodeList kids = way.childNodes();
    for (int k=0; k<kids.count(); k++) {
      QDomNode kid = kids.item(k);
      if (kid.isElement()) {
        QDomElement kidElt = kid.toElement();
        if (kidElt.tagName() == "tag") {
          QString key = kidElt.attribute("k");
          QString val = kidElt.attribute("v");
          db.WriteWayTag (wayid, key, val);
          savedtag ++;
        } else if (kidElt.tagName() == "nd") {
          QString nodeid = kidElt.attribute ("ref");
          db.WriteWayNode (wayid,nodeid);
          savednode ++;
        }
      }
    }
  }
  db.CommitTransaction ();
  int msecs = clock.elapsed ();
  mainUi.logDisplay->append (QString ("wrote %1 ways "
                                      "%2 tags %3 nodes in %4 msecs")
                            .arg (savedid)
                            .arg (savedtag)
                            .arg (savednode)
                            .arg (msecs));
}


Navi::Highway::Highway ()
  :ishighway(false)
{
}

void
Navi::Highway::clear ()
{
  name.clear ();
  kind.clear ();
  ishighway = false;
  attributes.clear();
}


} // namespace

