#include "sql-runner.h"

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

#include <QThreadPool>
#include <QTimer>
#include <QDebug>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlRecord>
#include "sql-run-query.h"
#include "sql-run-database.h"

namespace deliberate
{

SqlRunner::SqlRunner ()
  :doQuit (false),
   nextRequest (4242)
{
  wakeTimer = new QTimer (this);
  connect (wakeTimer, SIGNAL (timeout()), this, SLOT (Wake()));
}

SqlRunner::~SqlRunner ()
{
  Stop ();
  dbaseMap.clear ();
  queryMap.clear ();
  requestList.clear ();
}

void
SqlRunner::CloseAll ()
{
  requestLock.lock ();
  DBaseMapType::iterator dbit;
  QList<int> closeList;
  for (dbit = dbaseMap.begin(); dbit!= dbaseMap.end(); dbit++) {
    closeList.append (dbit.key());
  }
  requestLock.unlock ();
  for (int d=0; d<closeList.count(); d++) {
    CloseDB (d);
  }
}

bool
SqlRunner::CloseDB (int openId)
{
  bool ok (false);
  bool doemit (false);
  if (dbaseMap.contains (openId)) {
    QSqlDatabase * db = dbaseMap[openId]->sqlDB;
    if (db) {
      db->close();
      delete db;
      ok = true;
    }
    doemit = true;
    revDbaseMap.remove (dbaseMap[openId]);
    dbaseMap.remove (openId);
  }
  return doemit;
}

void
SqlRunner::Start (int priority)
{
  QThreadPool::globalInstance()->start (this, priority);
  wakeTimer->start (2000);
}

void
SqlRunner::Stop ()
{
  doQuit = true;
  idleWait.wakeAll();
}

void
SqlRunner::Wake ()
{
  idleWait.wakeAll ();
}

void
SqlRunner::run ()
{
  while (!doQuit) {
    idleLock.lock();   
    idleWait.wait (&idleLock);
    bool haveWork (true);
    while (haveWork) {
      doLock.lock();       // only one of these in body at one time
      haveWork = DoWork ();
      SendSignals ();
      doLock.unlock ();
    }
    idleLock.unlock ();
  }
  CloseAll ();
  return;
}

bool
SqlRunner::DoWork ()
{
  if (requestList.isEmpty ()) {
    return false;
  }
  RequestStruct req = requestList.takeFirst();
qDebug () << " DoWork " << req.type;
  switch (req.type) {
  case Req_Open:
qDebug () << " call DoOpen ";
    DoOpen (req);
    break;
  case Req_Close:
    DoClose (req);
    break;
  case Req_Exec:
    DoExec (req, false);
    break;
  case Req_ExecString:
    DoExec (req, true);
    break;
  case Req_Prepare:
    DoPrepare (req);
    break;
  case Req_Bind:
    DoBind (req);
    break;
  default:
    break;
  }
qDebug () << " back from switch ";
  return true;
}

void
SqlRunner::SendSignals ()
{
  while (!signalList.isEmpty()) {
    SignalStruct sig = signalList.takeFirst ();
    switch (sig.type) {
    case Sig_Op:
qDebug () << " emit Finished " << queryMap[sig.value0] << sig.okFlag;
      emit Finished (queryMap[sig.value0], sig.okFlag);
      break;
    case Sig_Open:
qDebug () << " emit Opened " << sig.value0 << dbaseMap[sig.value0] << sig.okFlag;
      emit Opened (dbaseMap[sig.value0], sig.okFlag);
      break;
    case Sig_Close:
qDebug () << " emit Closed " << dbaseMap[sig.value0];
      emit Closed (dbaseMap[sig.value0]);
      break;
    case Sig_Result:
qDebug () << " emit Finished " << dbaseMap[sig.value0] << sig.okFlag;
      emit Finished (queryMap[sig.value0], sig.okFlag);
      break;
    default:
      break;
    }
  }
}

void
SqlRunner::RequestOpen (int dbHandle)
{
  RequestStruct req (Req_Open);
  SqlRunDatabase * db = dbaseMap[dbHandle];
  req.data = db->dbName;
  requestLock.lock ();
  req.requestId = nextRequest++;
  req.openId = dbHandle;
  requestList.append (req);
  requestLock.unlock ();
  idleWait.wakeAll ();
}

void
SqlRunner::RequestClose (int openId)
{
  RequestStruct req (Req_Close);
  req.openId = openId;
  requestLock.lock ();
  req.requestId = nextRequest++;
  requestList.append (req);
  requestLock.unlock ();
  idleWait.wakeAll ();
}

void
SqlRunner::RequestExec (int queryHandle,
                        const QString & cmd)
{
qDebug () << "RequestExec " << cmd;
  QueueQuery (queryHandle, Req_ExecString, cmd);
}

void
SqlRunner::RequestExec (int prepareId)
{
  QueueQuery (prepareId, Req_Exec, QString());
}

void
SqlRunner::RequestPrepare (int queryHandle,
                                  const QString & cmd)
{
  QueueQuery (queryHandle, Req_Prepare, cmd);
}

void
SqlRunner::RequestBind (int prepareId,
                           int index,
                          const QVariant & value,
                QSql::ParamType paramType)
{
  RequestStruct req (Req_Bind);
  req.queryId = prepareId;
  req.requestId = nextRequest++;
  req.bindIndex = index;
  req.bindValue = value;
  req.bindType  = paramType;
  
  requestLock.lock ();
  requestList.append (req);
  requestLock.unlock ();
  idleWait.wakeAll ();
}

void
SqlRunner::RequestDelete (int queryHandle)
{
  RequestStruct req (Req_DeallocQuery);
  req.queryId = queryHandle;
  requestLock.lock ();
  requestList.append (req);
  requestLock.unlock ();
}

void
SqlRunner::QueueQuery (int queryHandle,
                          RequestType type,
                                const QString & cmd)
{
  RequestStruct req (type);
  bool ok (false);
qDebug () << " Queue Query " << cmd;
  requestLock.lock ();
qDebug () << "    have lock ";
  if (queryMap.contains (queryHandle)) {
    SqlRunQuery * query = queryMap[queryHandle];
    req.requestId = nextRequest++;
    req.queryId = queryHandle;
    req.openId = query->dbHandle;
    req.data = cmd;
    ok = true;
  }
  if (ok) {
    requestList.append (req);
  }
  requestLock.unlock ();
  idleWait.wakeAll ();
qDebug () << " unlocked ";
}

void
SqlRunner::DoOpen (RequestStruct & req)
{
qDebug () << " DoOpen ";
  QSqlDatabase *db = new QSqlDatabase ;
  (*db) = QSqlDatabase::addDatabase ("QSQLITE");
  db->setDatabaseName (req.data);
qDebug () << " db name " << req.data;
qDebug () << " db ptr " << db;
  bool ok = db->open ();
  if (ok) {
    SqlRunDatabase * sdb = dbaseMap[req.openId];
    sdb->sqlDB = db;
  } else {
    delete db;
  }
  SignalStruct sig (Sig_Open);
  sig.okFlag = ok;
  sig.value0 = req.openId;
qDebug () << " open for " << req.data << " ok " << ok;
  signalList.append (sig);
}

void
SqlRunner::DoClose (RequestStruct &req)
{
  if (CloseDB (req.openId)) {
    SignalStruct sig (Sig_Close);
    sig.value0 = req.openId;
    signalList.append (sig);
  }
}

void
SqlRunner::DoExec (RequestStruct & req, bool useString)
{
  SignalStruct sig (Sig_Result);
  sig.value0 = req.queryId;
  if (queryMap.contains (req.queryId)) {
    SqlRunQuery * qry = queryMap[req.queryId];
    if (!qry) {
      return;     // corrupted map
    }
    bool ok;
    qry->execStarted = true;
qDebug () << " try query " << req.data;
    qry->result.clear ();
    if (useString) {
      ok = qry->sqlQuery->exec (req.data);
    } else {
      ok = qry->sqlQuery->exec ();
    }
    sig.okFlag = ok;
    BuildResults (qry);
    qry->finished = true;
    qry->didQuery = qry->sqlQuery->executedQuery();
  } else {
    sig.okFlag = false;
  }
  signalList.append (sig);
}

void
SqlRunner::DoPrepare (RequestStruct & req)
{
  if (queryMap.contains (req.queryId)) {
    QSqlQuery * qry = queryMap[req.queryId]->sqlQuery;
    if (!qry) {
      return;     // corrupted map
    }
    qry->prepare (req.data);
  }
}

void
SqlRunner::DoBind (RequestStruct & req)
{
  if (queryMap.contains (req.queryId)) {
    QSqlQuery * qry = queryMap[req.queryId]->sqlQuery;
    if (!qry) {
      return;     // corrupted map
    }
    qry->bindValue (req.bindIndex, req.bindValue, req.bindType);
  }
}

void
SqlRunner::DoDealloc (RequestStruct & req)
{
  if (queryMap.contains (req.queryId)) {
    SqlRunQuery * qry = queryMap[req.queryId];
    if (qry) {
      QSqlQuery * sqry = qry->sqlQuery;
      if (sqry) {
        delete sqry;
      }
      delete qry;
    }
    queryMap.remove (req.queryId);
  }
}

void
SqlRunner::BuildResults (SqlRunQuery * query)
{
  QSqlQuery * sqry = query->sqlQuery;
  if (sqry == 0) {
    return;
  }
  QSqlRecord rec = sqry->record();
  int cols = rec.count();
  int numRows (0);
  while (sqry->next()) {
    QVector<QVariant> row(cols);
    for (int c=0; c<cols; c++) {
      row[c] = sqry->value (c);
    }
    query->result.append (row);
    numRows++;
  }
  query->numRows = numRows;
}

SqlRunQuery*
SqlRunner::newQuery (SqlRunDatabase * db)
{
  if (revDbaseMap.contains (db)) {
    QSqlQuery *sqry = new QSqlQuery (*(db->sqlDB));
    int dbHandle = db->openId;
    int queryHandle = nextRequest++;
    SqlRunQuery * qry = new SqlRunQuery (this, sqry, 
                            dbHandle, queryHandle);
    requestLock.lock();
    queryMap[queryHandle] = qry;
    requestLock.unlock();
    return qry;
  } else {
    return 0;
  }
}

SqlRunDatabase*
SqlRunner::openDatabase (const QString & filename)
{
  SqlRunDatabase * newdb = new SqlRunDatabase (this);
  requestLock.lock ();
  int dbid = nextRequest++;
  newdb->openId = dbid;
  newdb->dbName = filename;
  newdb->sqlDB = 0;
  dbaseMap[dbid] = newdb;
  revDbaseMap[newdb] = dbid;
  requestLock.unlock ();
  RequestOpen (dbid);
  return newdb;
}

SqlRunDatabase*
SqlRunner::database (int dbHandle)
{
  if (dbaseMap.contains (dbHandle)) {
    return dbaseMap[dbHandle];
  } else {
    return 0;
  }
}

} // namespace
