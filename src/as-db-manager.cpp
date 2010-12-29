#include "as-db-manager.h"


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
#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QDesktopServices>
#include <QTimer>
#include <QByteArray>
#include "deliberate.h"

#include <QDebug>

using namespace deliberate;

namespace navi
{

AsDbManager::AsDbManager (QObject *parent)
  :QObject (parent),
   geoBaseHandle (-1)
{
  runner = new SqliteRunner;
  Connect ();
}

AsDbManager::~AsDbManager ()
{
  runner->Stop ();
  delete runner;
  runner = 0;
}

void
AsDbManager::Start ()
{
  QString dataDir = QDesktopServices::storageLocation
                    (QDesktopServices::DataLocation);
  QString geoBaseName = dataDir + QDir::separator() 
                         + QString ("geobase.sql");
  geoBaseName = Settings().simpleValue ("database/geobase",geoBaseName)
                                    .toString();
  Settings().setSimpleValue ("database/geobase",geoBaseName);
  
  QStringList  geoElements;
  geoElements   << "nodes"
                << "nodelatindex"
                << "nodelonindex"
                << "ways"
                << "nodetags"
                << "waytags"
                << "waynodes"
                << "nodeparcels"
                << "wayparcels"
                << "relations"
                << "relationparts"
                << "relationtags";

  runner->Start ();
  StartDB (geoBaseHandle, geoBaseName);
  CheckDBComplete (geoBaseHandle, geoElements);
}

void
AsDbManager::Connect ()
{
  connect (runner, SIGNAL (DoneOpen (int , bool)),
           this, SLOT (CatchOpen (int, bool)));
  connect (runner, SIGNAL (DoneClose (int)),
           this, SLOT (CatchClose (int)));
  connect (runner, SIGNAL (DoneOp (int, bool, int)),
           this, SLOT (CatchOp (int, bool, int)));
  connect (runner, SIGNAL (ResultsReady (int, bool, const QVariant&)),
           this, SLOT (CatchResults (int, bool, const QVariant&)));
}

void
AsDbManager::Stop ()
{
  qDebug () << " AsDbManager Stop";
  runner->Stop ();
}

void
AsDbManager::StartDB (int & handle, const QString & dbname)
{
  CheckFileExists (dbname);
  handle = runner->RequestOpen (dbname);
  DbState state;
  state.open = false;
  state.name = dbname;
  state.checkInProgress = false;
  dbMap[handle] = state;
}

void
AsDbManager::CheckFileExists (const QString & filename)
{
  QFileInfo info (filename);
  if (!info.exists ()) {
    QDir  dir (info.absolutePath ());
    dir.mkpath (info.absolutePath ());
    QFile  file (filename);
    file.open (QFile::ReadWrite);
    file.write (QByteArray (""));
    file.close();
  }
}

void
AsDbManager::CheckDBComplete (int dbHandle, const QStringList & elements)
{
  dbCheckList[dbHandle] = elements;
  dbMap[dbHandle].checkInProgress = true;
}

void
AsDbManager::CatchOpen (int dbHandle, bool ok)
{
  if (dbMap.contains (dbHandle)) {
    dbMap[dbHandle].open = ok;
    if (dbMap[dbHandle].checkInProgress) {
      ContinueCheck (dbHandle);
    }
  }
}

void
AsDbManager::CatchClose (int dbHandle)
{
  QMessageBox box;
  box.setText (QString("Ignore Close db handle %1").arg(dbHandle));
  QTimer::singleShot (10000, &box, SLOT (accept()));
  box.exec ();
}

void
AsDbManager::CatchOp (int queryHandle, bool ok, int numRowsAffected)
{
  QMessageBox box;
  box.setText (QString("Ignore Op Complete query %1 ok %2")
                      .arg(queryHandle)
                      .arg (ok));
  QTimer::singleShot (10000, &box, SLOT (accept()));
  box.exec ();
}

void
AsDbManager::CatchResults (int queryHandle, bool ok, 
                          const QVariant & results)
{
  QMessageBox box;
  box.setText (QString("Ignore Results for query %1 ok %2")
                      .arg(queryHandle)
                      .arg (ok));
  QTimer::singleShot (10000, &box, SLOT (accept()));
  box.exec ();
  if (!queryMap.contains(queryHandle)) {
    return;  // ignore bad results
  }
  QueryType type = queryMap[queryHandle].type;
  switch (type) {
  case Query_IgnoreResult:
     break;
  case Query_AskElement:
     CheckElementType (queryHandle, ok, results);
     break;
  default:
     qDebug () << " Not Handling Query " << type;
     break;
  }
  queryMap.remove (queryHandle);
}

void
AsDbManager::ContinueCheck (int dbHandle)
{
  if (!dbCheckList[dbHandle].isEmpty()) {
    QString element = dbCheckList[dbHandle].takeFirst();
    AskElementType (dbHandle, element);
  }
}

void
AsDbManager::AskElementType (int dbHandle, const QString & eltName)
{
  QString pat ("select * from main.sqlite_master where name =\"%1\"");
  QueryState query;
  query.finished = false;
  query.type = Query_AskElement;
  query.dbHandle = dbHandle;
  query.data = QVariant (eltName);
  int queryHandle = runner->RequestSelectExec (dbHandle, 
                       1, pat.arg(eltName));
  queryMap[queryHandle] = query;
}

void
AsDbManager::CheckElementType (int queryHandle, bool ok, 
                              const QVariant & results)
{
  bool typeGood (ok);
qDebug () << " Element Type result " << results;
  if (typeGood) {
    QString eltType = results.toMap()["0"].toMap()["0"].toString();
    typeGood = (eltType == "TABLE" || eltType == "INDEX");
  }
  QString eltName = queryMap[queryHandle].data.toString();
  int dbHandle = queryMap[queryHandle].dbHandle;
  dbCheckList[dbHandle].removeAll (eltName);
  if (!typeGood) {
    MakeElement (dbHandle, eltName);
  }
  ContinueCheck (dbHandle);
}

void
AsDbManager::MakeElement (int dbHandle, const QString & elementName)
{
  QString filename (QString (":/schemas/%1.sql").arg (elementName));
  QFile  schemafile (filename);
  schemafile.open (QFile::ReadOnly);
  QByteArray  createcommands = schemafile.readAll ();
  schemafile.close ();
  QString cmd (createcommands);
  QueryState query;
  query.finished = false;
  query.type = Query_IgnoreResult;
  int handle = runner->RequestQuery (dbHandle, cmd);
  queryMap[handle] = query;
}

} // namespace
