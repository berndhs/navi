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
qDebug () << "AsDbManager in thread " << QThread::currentThread();
  runner = new SqlRunner;
  Connect ();
}

AsDbManager::~AsDbManager ()
{
  runner->Stop ();
  delete runner;
  runner = 0;
}

int
AsDbManager::PendingRequestCount ()
{
  if (runner) {
    return queryMap.count ();
  } else {
    return -1;
  }
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
                << "waylocs"
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
           this, SLOT (CatchOpen (SqlRunDatabase* , bool)),
           Qt::QueuedConnection);
  connect (runner, SIGNAL (Closed (SqlRunDatabase* )),
           this, SLOT (CatchClose (SqlRunDatabase* )),
           Qt::QueuedConnection);
  connect (runner, SIGNAL (Finished  (SqlRunQuery*, bool)),
           this, SLOT (CatchFinished (SqlRunQuery*, bool)),
           Qt::QueuedConnection);
  connect (runner, SIGNAL (MarkReached (int, bool)),
           this, SLOT (CatchMark (int, bool)),
           Qt::QueuedConnection);
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
AsDbManager::CatchMark (int markId, bool ok)
{
  emit MarkReached (markId);
}


void
AsDbManager::CatchFinished (SqlRunQuery *query, bool ok)
{
qDebug () << " Catch Finished " << ok << query->executedQuery();
qDebug () << " Catch Finished in thread " << QThread::currentThread();
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
  case Query_AskWayList:
     ReturnWayList (query, ok);
     break;
  case Query_AskWayTurnList:
     ReturnWayTurnList (query, ok);
     break;
  case Query_RangeNodeTags:
     ReturnRangeNodeTags (query, ok);
     break;
  case Query_CreateTemp:
     ReturnTemp (query, ok);
     break;
  default:
     qDebug () << " Finishe Not Handling Query " << type;
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
  QString cmd ("select nodeid, lat, lon from nodes where "
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
AsDbManager::AskNodes (const QString & tablePrefix)
{
  SqlRunQuery *query = runner->newQuery(geoBase);
  QString cmd ("select nodeid, lat, lon from %1_nodes  ");
  QueryState qstate;
  qstate.type = Query_AskRangeNodes;
  int reqId = nextRequest++;
  qstate.reqId = reqId;
  qstate.db = geoBase;
  queryMap[query] = qstate;
  query->exec (cmd.arg (tablePrefix));
qDebug () << " sent query " << cmd.arg(tablePrefix);
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
  qstate.reqId = reqId;
  qstate.db = geoBase;
  queryMap[query] = qstate;
  query->exec (cmd.arg(nodeid));
  return reqId;
}

int
AsDbManager::AskWaysByNode (const QString & nodeId)
{
  SqlRunQuery * query = runner->newQuery (geoBase);
  if (!query) {
    qDebug () << "Query allocation failure";
    return -1;
  }
  QString cmd ("select wayid from waynodes where nodeid = \"%1\"");
  QueryState qstate;
  qstate.type = Query_AskWayList;
  int reqId = nextRequest++;
  qstate.reqId = reqId;
  qstate.db = geoBase;
  queryMap[query] = qstate;
  query->exec (cmd.arg (nodeId));
  return reqId;
}

int
AsDbManager::AskWaysByTag (const QString & key, const QString & value,
                          bool regular)
{
  SqlRunQuery * query = runner->newQuery (geoBase);
  if (!query) {
    qDebug () << "Query allocation failure";
    return -1;
  }
  QString cmd ("select wayid from waytags where "
               " key = \"%1\" AND value %2 \"%3\"");
  QString op (regular ? "GLOB" : "=");
  QueryState qstate;
  qstate.type = Query_AskWayList;
  int reqId = nextRequest;
  qstate.reqId = reqId;
  queryMap[query] = qstate;
  query->exec (cmd.arg (key).arg(op).arg(value));
  return reqId;
}
  

int
AsDbManager::SetRange (QString & tablePrefix, 
                      double south, double west, 
                      double north, double east)
{
  static int tempnum (1);
  QString createTmp ("create  temporary table %5 as "
               " select nodeid, lat, lon from nodes where "
               " lat >= %1 AND lat <= %2 "
               " AND "
               " lon >= %3 AND lon <= %4 ");
  tablePrefix = QString ("TR%1").arg(tempnum++);
  QString tmpname (QString ("%1_nodes").arg (tablePrefix));
  SqlRunQuery * tmpCreate = runner->newQuery (geoBase);
  QueryState qstate (nextRequest++, Query_CreateTemp, geoBase);
  int reqId = qstate.reqId;
  queryMap[tmpCreate] = qstate;
  QString realCmd = createTmp.arg (south).arg (north)
                            .arg (west).arg (east)
                            .arg (tmpname);
qDebug () << " real Command " << realCmd;
  tmpCreate->exec (realCmd);
  return reqId;
}

int
AsDbManager::GetRangeWays (const QString & prefix,
                         double south, double west, 
                      double north, double east)
{
  QString createTmp ("create temporary table %1 as "
                     " select wayid, nodeid, seq, lat, lon from waylocs where "
               " lat >= %2 AND lat <= %3 "
               " AND "
               " lon >= %4 AND lon <= %5 ");
  QString tmpname (QString ("%1_waylocs").arg (prefix));
  SqlRunQuery * tmpCreate = runner->newQuery (geoBase);
  QueryState qstate1 (nextRequest++, Query_CreateTemp, geoBase);
  queryMap[tmpCreate] = qstate1;

  QString selectAll ("select wayid, nodeid, seq, lat, lon from %1");
  SqlRunQuery * select = runner->newQuery (geoBase);
  QueryState qstate2 (nextRequest++, Query_AskWayTurnList, geoBase);
  queryMap[select] = qstate2;
  int reqId = qstate2.reqId;
  tmpCreate->exec (createTmp.arg (tmpname).arg (south).arg (north)
                             .arg (west).arg(east));
  select->exec (selectAll.arg (tmpname));
  return reqId;
}

#if 0
int
AsDbManager::AskRangeNodeTags (double south, double west, 
                      double north, double east)
{
  static int tempnum (1);
  QString createTmp1 ("create temporary table %5 as "
               " select nodeid from nodes where "
               " lat >= %1 AND lat <= %2 "
               " AND "
               " lon >= %3 AND lon <= %4 ");
  QString tmpname1 (QString ("tempRNT%1").arg(tempnum++));
  SqlRunQuery * tmpCreate1 = runner->newQuery (geoBase);
  QueryState qstate1 (nextRequest++, Query_IgnoreResult, geoBase);

  QString createTmp2 ("create temporary table %1 as "
                  " select nodetags.nodeid, key, value from nodetags "
                  " inner join %2 on "
                  " nodetags.nodeid = %2.nodeid ");
  QString tmpname2 (QString ("tempRNT%1").arg(tempnum++));
  SqlRunQuery * tmpCreate2 = runner->newQuery (geoBase);
  QueryState qstate2 (nextRequest++, Query_IgnoreResult, geoBase);

  QString selectCmd ("select nodeid, key, value from %1");
  SqlRunQuery * select = runner->newQuery (geoBase);
  QueryState qstate3 (nextRequest++, Query_RangeNodeTags, geoBase);
  int reqId = qstate3.reqId;

  QString dropCmd ("drop table %1");
  SqlRunQuery * drop1 = runner->newQuery (geoBase);
  QueryState qstate4 (nextRequest++, Query_IgnoreResult, geoBase);
  SqlRunQuery * drop2 = runner->newQuery (geoBase);
  QueryState qstate5 (nextRequest++, Query_IgnoreResult, geoBase);
  queryMap[tmpCreate1] = qstate1;
  queryMap[tmpCreate2] = qstate2;
  queryMap[select] = qstate3;
  queryMap[drop1] = qstate4;
  queryMap[drop2] = qstate5;
  tmpCreate1->exec (createTmp1.arg (south).arg(north).arg(west).arg(east)
                             .arg (tmpname1));
  tmpCreate2->exec (createTmp2.arg (tmpname2).arg (tmpname1));
  select->exec (selectCmd.arg (tmpname2));
  drop1->exec (dropCmd.arg (tmpname1));
  drop2->exec (dropCmd.arg (tmpname2));
  return reqId;
}
#endif

void
AsDbManager::ReturnRangeNodes (SqlRunQuery * query, bool ok)
{
  NaviNodeList nodeList;
  if (ok && query) {
    while (query->next()) {
      QString id = query->value(0).toString();
      double lat = query->value(1).toDouble();
      double lon = query->value(2).toDouble();
      nodeList.append (NaviNode (id, lat, lon));
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
qDebug () << " return tag list for req " << reqId;
  emit HaveTagList (reqId, tagList);
}

void
AsDbManager::ReturnWayList (SqlRunQuery * query, bool ok)
{
  QStringList wayList;
  if (ok && query) {
    while (query->next ()) {
      wayList.append (query->value(0).toString());
    }
  }
  int reqId = queryMap[query].reqId;
  emit HaveWayList (reqId, wayList);
}

void
AsDbManager::ReturnWayTurnList (SqlRunQuery * query, bool ok)
{
  WayTurnList wayList;
  if (ok && query) {
    while (query->next ()) {
      WayTurn turn (query->value(0).toString(),
                    query->value(1).toString(),
                    query->value(2).toInt(),
                    query->value(3).toDouble(),
                    query->value(4).toDouble ());
      wayList.append (turn);
    }
  }
  int reqId = queryMap[query].reqId;
  emit HaveWayTurnList (reqId, wayList);
}

void
AsDbManager::ReturnRangeNodeTags (SqlRunQuery * query, bool ok)
{
  TagRecordList list;
  if (ok && query) {
    while (query->next ()) {
      list.append (TagRecord (query->value(0).toString(),
                              query->value(1).toString(),
                              query->value(2).toString()));
    }
  }
  emit HaveRangeNodeTags (queryMap[query].reqId, list);
}

void
AsDbManager::ReturnTemp (SqlRunQuery * query, bool ok)
{
  emit HaveTemp (queryMap[query].reqId, ok);
}

void
AsDbManager::WriteNode (const QString & nodeId,
                         double lat,
                         double lon)
{
  SqlRunQuery *insert = runner->newQuery(geoBase);
  QString cmd ("insert or replace into nodes "
               " nodeid, lat, lon) "
               " VALUES (\"%1\", %2, %3)");
  insert->exec (cmd.arg(nodeId).arg(lat).arg(lon)); 
}

void
AsDbManager::WriteWay (const QString & wayId)
{
  SqlRunQuery *insert = runner->newQuery(geoBase);
  QString cmd ("insert or replace into ways "
               " (wayid) "
               " VALUES (\"%1\")");
  insert->exec (cmd.arg(wayId)); 
}

void
AsDbManager::WriteRelation (const QString & relationId)
{
  SqlRunQuery *insert = runner->newQuery(geoBase);
  QString cmd ("insert or replace into relations "
               " (relationid, lat, lon) "
               " VALUES (\"%1\")");
  insert->exec (cmd.arg(relationId)); 
}

void
AsDbManager::WriteNodeParcel (const QString & nodeId, 
                            quint64 parcelIndex)
{
  WriteParcel ("node",nodeId, parcelIndex);
}

void
AsDbManager::WriteWayParcel (const QString & wayId,
                           quint64 parcelIndex)
{
  WriteParcel ("way", wayId, parcelIndex);
}


void
AsDbManager::WriteParcel (const QString & type,
                        const QString & id,
                        quint64 parcelIndex)
{
  QString cmd ("insert or replace into %1parcels "
               " (%1id, parcelid) "
               " VALUES (\"%2\", \"%3\")");
  SqlRunQuery * insert = runner->newQuery (geoBase);
  insert->exec (cmd.arg (type).arg(id).arg(parcelIndex));
}

void
AsDbManager::WriteWayNode (const QString & wayId,
                         const QString & nodeId)
{
  QString cmd ("insert or replace into waynodes "
               " (wayid, nodeid) "
               " VALUES (\"%1\", \"%2\") ");
  SqlRunQuery * insert = runner->newQuery (geoBase);
  insert->exec (cmd.arg(wayId).arg(nodeId));
}

void
AsDbManager::WriteNodeTag (const QString & nodeId, 
                     const QString & key,
                     const QString & value)
{
  WriteTag ("node",nodeId, key, value);
}

void
AsDbManager::WriteWayTag (const QString & wayId, 
                     const QString & key,
                     const QString & value)
{
  WriteTag ("way",wayId, key, value);
}

void
AsDbManager::WriteRelationTag (const QString & relId,
                     const QString & key,
                     const QString & value)
{
  WriteTag ("relation", relId, key, value);
}

void
AsDbManager::WriteTag (const QString & type,
                     const QString & id,
                     const QString & key,
                     const QString & value)
{
  QString cmd ("insert or replace into %1tags "
               " (%1id, key, value) "
               " VALUES (\"%2\", \"%3\", \"%4\")");
  SqlRunQuery *insert = runner->newQuery(geoBase);
  insert->exec (cmd.arg (type).arg(id).arg(key).arg(value));
}

void
AsDbManager::WriteRelationMember (const QString & relId,
                                const QString & type,
                                const QString & ref)
{
  QString cmd ("insert or replace into relationparts "
               "  (relationid, othertype, otherid) "
               " VALUES (\"%1\", \"%2\", \"%3\") ");
  SqlRunQuery *insert = runner->newQuery(geoBase);
  insert->exec (cmd.arg(relId).arg(type).arg(ref));
}

void
AsDbManager::StartTransaction ()
{
  if (geoBase) {
    geoBase->transaction();
  }
}

void
AsDbManager::CommitTransaction ()
{
  if (geoBase) {
    geoBase->commit ();
  }
}

int
AsDbManager::SetMark ()
{
  int mark = runner->Mark ();
  return mark;
}

} // namespace
