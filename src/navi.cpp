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
#include <QCursor>
#include <QNetworkRequest>
#include <QDomDocument>
#include <QDomElement>
#include <QDomNode>
#include <QDomNodeList>


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
  return true;
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
  double lat = mainUi.latEdit->text().toDouble ();
  double lon = mainUi.lonEdit->text().toDouble ();
  QUrl url;
  url.setScheme ("http");
  url.setHost ("xapi.openstreetmap.org");
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
  mainUi.logDisplay->append (QString ("requested: %1")
                               .arg (req.url().toString()));
}

void
Navi::HandleReply (QNetworkReply * reply)
{
  QByteArray  data = reply->readAll ();
  mainUi.logDisplay->append ("Reply to ");
  mainUi.logDisplay->append (reply->url().toString());
  mainUi.logDisplay->append (QString (data));
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
  QDomNodeList kids = node.childNodes ();
  if (node.isElement ()) {
    QDomElement elt = node.toElement();
    QString id = elt.attribute ("id");
    mainUi.logDisplay->append (QString ("Way ID %1").arg (id));
  } else {
    mainUi.logDisplay->append ("Way Node not and Element");
  }
  for (int k=0; k<kids.count(); k++) {
    QDomNode kid = kids.item(k);
    if (kid.isElement()) {
      QDomElement kidElt = kid.toElement();
      if (kidElt.tagName() == "tag") {
        QString key = kidElt.attribute("k");
        QString val = kidElt.attribute("v");
        mainUi.logDisplay->append (QString ("  tag key \"%1\""
                                            "  value \"%2\"")
                                  .arg (key).arg (val));
      } 
    }
  }
}



} // namespace

