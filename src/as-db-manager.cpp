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
#include <QStringList>
#include "deliberate.h"
#include "sql-run-database.h"
#include "sql-run-query.h"

#include <QDebug>

using namespace deliberate;

namespace navi
{

AsDbManager::AsDbManager (QObject *parent)
  :QObject (parent),
   geoBase (0),
   nextRequest (111)
{
  runner = new SqlRunner;
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
  geoBase = StartDB (geoBaseName);
qDebug () << " stated DB " << geoBase;
  CheckDBComplete (geoBase, geoElements);
}

void
AsDbManager::Connect ()
{
  connect (runner, SIGNAL (Opened (SqlRunDatabase* , bool)),
           this, SLOT (CatchOpen (SqlRunDatabase* , bool)));
  connect (runner, SIGNAL (Closed (SqlRunDatabase* )),
           this, SLOT (CatchClose (SqlRunDatabase* )));
  connect (runner, SIGNAL (Finished  (SqlRunQuery*, bool)),
           this, SLOT (CatchFinished (SqlRunQuery*, bool)));
}

void
AsDbManager::Stop ()
{
  qDebug () << " AsDbManager Stop";
  runner->Stop ();
}

SqlRunDatabase*
AsDbManager::StartDB (const QString & dbname)
{
  CheckFileExists (dbname);
  SqlRunDatabase *db = runner->openDatabase (dbname);
  DbState state;
  state.open = false;
  state.name = dbname;
  state.checkInProgress = false;
  dbMap[db] = state;
  return db;
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
AsDbManager::CheckDBComplete (SqlRunDatabase *db, 
                           const QStringList & elements)
{
  dbCheckList[db] = elements;
  dbMap[db].checkInProgress = true;
}

void
AsDbManager::CatchOpen (SqlRunDatabase *db, bool ok)
{
  if (dbMap.contains (db)) {
    dbMap[db].open = ok;
    if (dbMap[db].checkInProgress) {
      ContinueCheck (db);
    }
  }
}

void
AsDbManager::CatchClose (SqlRunDatabase *db)
{
  Q_UNUSED (db)
  QMessageBox box;
  box.setText (QString("Ignore Close"));
  QTimer::singleShot (10000, &box, SLOT (accept()));
  box.exec ();
}

void
AsDbManager::CatchFinished (SqlRunQuery *query, bool ok)
{
qDebug () << " Catch Finished " << query;
  if (!queryMap.contains(query)) {
qDebug () << " Finishe unknown query " << query;
    return;  // ignore bad results
  }
  QueryType type = queryMap[query].type;
qDebug () << " Finishe type " << type;
  switch (type) {
  case Query_IgnoreResult:
     break;
  case Query_AskElement:
     CheckElementType (query,ok);
     break;
  case Query_AskRangeNodes:
     ReturnRangeNodes (query, ok);
     break;
  case Query_AskLatLon:
     ReturnLatLon (query, ok);
     break;
  case Query_AskTagList:
     ReturnTagList (query, ok);
     break;
  default:
     qDebug () << " Not Handling Query " << type;
     break;
  }
  query->deleteLater();
  queryMap.remove (query);
}

void
AsDbManager::ContinueCheck (SqlRunDatabase * db)
{
  if (!dbCheckList[db].isEmpty()) {
    QString element = dbCheckList[db].takeFirst();
    AskElementType (db, element);
  }
}

void
AsDbManager::AskElementType (SqlRunDatabase * db, const QString & eltName)
{
  QString pat ("select * from main.sqlite_master where name =\"%1\"");
  QueryState qstate;
  qstate.finished = false;
  qstate.type = Query_AskElement;
  qstate.db = db;
  qstate.data = QVariant (eltName);
  SqlRunQuery * query = runner->newQuery (db);
  queryMap[query] = qstate;
  query->exec (pat.arg(eltName));
}

void
AsDbManager::CheckElementType (SqlRunQuery *query, bool ok)
{
  bool typeGood (ok);
  if (typeGood) {
    query->first();
    QString eltType = query->value(0).toString();
    typeGood = (eltType == "TABLE" || eltType == "INDEX");
  }
  QString eltName = queryMap[query].data.toString();
  SqlRunDatabase * db = queryMap[query].db;
  dbCheckList[db].removeAll (eltName);
  if (!typeGood) {
    MakeElement (db, eltName);
  }
  ContinueCheck (db);
}

void
AsDbManager::MakeElement (SqlRunDatabase * db, const QString & elementName)
{
  QString filename (QString (":/schemas/%1.sql").arg (elementName));
  QFile  schemafile (filename);
  schemafile.open (QFile::ReadOnly);
  QByteArray  createcommands = schemafile.readAll ();
  schemafile.close ();
  QString cmd (createcommands);
  QueryState qstate;
  qstate.finished = false;
  qstate.type = Query_IgnoreResult;
  SqlRunQuery * query = runner->newQuery (db);
  queryMap[query] = qstate;
  query->exec (cmd);
}

int
AsDbManager::AskRangeNodes (double south, double west,
                            double north, double east)
{
  SqlRunQuery *query = runner->newQuery(geoBase);
  QString cmd ("select nodeid from nodes where "
               " lat >= %1 AND lat <= %2 "
               " AND "
               " lon >= %3 AND lon <= %4 ");
  QueryState qstate;
  qstate.type = Query_AskRangeNodes;
  int reqId = nextRequest++;
  qstate.reqId = reqId;
  qstate.db = geoBase;
  queryMap[query] = qstate;
  query->exec (cmd.arg (south).arg(north).arg (west).arg(east));
  return reqId;
}

int
AsDbManager::AskLatLon (const QString & nodeid)
{
  SqlRunQuery *query = runner->newQuery(geoBase);
  if (!query) {
    qDebug () << "QUery allocation failed";
    return -1;
  }
  QString cmd ("select lat, lon from nodes where nodeid=\"%1\"");
  QueryState qstate;
  qstate.type = Query_AskLatLon;
  int reqId = nextRequest++;
  qstate.reqId = reqId;
  qstate.db = geoBase;
  queryMap[query] = qstate;
  query->exec (cmd.arg(nodeid));
  return reqId;
}

int
AsDbManager::AskNodeTagList (const QString & nodeid)
{
  SqlRunQuery *query = runner->newQuery(geoBase);
  if (!query) {
    qDebug () << "QUery allocation failed";
    return -1;
  }
  QString cmd ("select key, value from nodetags where nodeid=\"%1\"");
  QueryState qstate;
  qstate.type = Query_AskTagList;
  int reqId = nextRequest++;
  qstate.db = geoBase;
  queryMap[query] = qstate;
  query->exec (cmd.arg(nodeid));
  return reqId;
}

void
AsDbManager::ReturnRangeNodes (SqlRunQuery * query, bool ok)
{
  QStringList nodeList;
  if (ok && query) {
    while (query->next()) {
      nodeList.append (query->value(0).toString());
    }
  }
  int reqId = queryMap[query].reqId;
  emit HaveRangeNodes (reqId, nodeList);
}

void
AsDbManager::ReturnLatLon (SqlRunQuery * query, bool ok)
{
  double lat (0.0);
  double lon (0.0);
  if (ok && query) {
    if (query->next()) {
      lat = query->value(0).toDouble();
      lon = query->value(1).toDouble();
    }
  }
  int reqId = queryMap[query].reqId;
  emit HaveLatLon (reqId, lat, lon);
}

void
AsDbManager::ReturnTagList (SqlRunQuery * query, bool ok)
{
  TagList tagList;
  if (ok && query) {
    while (query->next()) {
      TagItemType tag;
      tag.first = query->value (0).toString();
      tag.second = query->value (1).toString ();
      tagList.append (tag);
    }
  }
  int reqId = queryMap[query].reqId;
  emit HaveTagList (reqId, tagList);
}

} // namespace
