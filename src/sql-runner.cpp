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
#include <QThread>
#include "sql-run-query.h"
#include "sql-run-database.h"

namespace deliberate
{

SqlRunner::SqlRunner ()
  :doQuit (false),
   doneQuit (false),
   nextRequest (4242)
{
  wakeTimer = new QTimer (this);
  setAutoDelete (false);
  connect (wakeTimer, SIGNAL (timeout()), this, SLOT (Wake()));
}

SqlRunner::~SqlRunner ()
{
  Stop ();
  dbaseMap.clear ();
  queryMap.clear ();
  requestList.clear ();
}

int
SqlRunner::PendingRequests ()
{
  return requestList.count();
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
  if (!doneQuit) {
    doQuit = true;
    idleWait.wakeAll();
  }
}

void
SqlRunner::Wake ()
{
  if (!doneQuit) {
    WakeAll ();
  }
}

void
SqlRunner::run ()
{
  doneQuit = false;
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
  doneQuit = true;
  return;
}

bool
SqlRunner::DoWork ()
{
  requestLock.lock ();
  if (requestList.isEmpty ()) {
    requestLock.unlock ();
    return false;
  }
  RequestStruct req = requestList.takeFirst();
  requestLock.unlock ();
  switch (req.type) {
  case Req_Open:
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
  case Req_DeallocQuery:
    DoDealloc (req);
    break;
  default:
    break;
  }
  return true;
}

void
SqlRunner::SendSignals ()
{
  while (!signalList.isEmpty()) {
    SignalStruct sig = signalList.takeFirst ();
    switch (sig.type) {
    case Sig_Op:
      emit Finished (queryMap[sig.value0], sig.okFlag);
      break;
    case Sig_Open:
      emit Opened (dbaseMap[sig.value0], sig.okFlag);
      break;
    case Sig_Close:
      emit Closed (dbaseMap[sig.value0]);
      break;
    case Sig_Result:
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
  WakeAll ();
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
  WakeAll ();
}

void
SqlRunner::RequestExec (int queryHandle,
                        const QString & cmd)
{
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
  WakeAll ();
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
  static int callCount (0);
  RequestStruct req (type);
  bool ok (false);
  requestLock.lock ();
  callCount++;
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
  bool yield (false);
  if (callCount > 32) {
    callCount = 0;
    yield = true;
  }
  WakeAll (yield);
}

void
SqlRunner::DoOpen (RequestStruct & req)
{
  QSqlDatabase *db = new QSqlDatabase ;
  (*db) = QSqlDatabase::addDatabase ("QSQLITE");
  db->setDatabaseName (req.data);
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
SqlRunner::DoDealloc (RequestStruct & req)
{
  if (queryMap.contains (req.queryId)) {
    SqlRunQuery * qry = queryMap[req.queryId];
    queryMap.remove (req.queryId);
    delete qry;
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
SqlRunner::BuildResults (SqlRunQuery * query)
{
  QSqlQuery * sqry = query->sqlQuery;
  if (sqry == 0) {
    return;
  }
  QSqlRecord rec = sqry->record();
  int cols = rec.count();
  int numRows (0);
  query->currentRow = -1;
  while (sqry->next()) {
    QMap<int, QVariant> row;
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
  requestLock.lock();
  QSqlDatabase * sdb = db->sqlDB;
  if (!sdb) {
    qFatal ( "no SQL Database in runner");
  }
  int dbHandle = db->openId;
  int queryHandle = nextRequest++;
  SqlRunQuery * qry = new SqlRunQuery (this, sdb, 
                          dbHandle, queryHandle);
  queryMap[queryHandle] = qry;
  requestLock.unlock();
  return qry;
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

void
SqlRunner::WakeAll (bool yield)
{
  idleWait.wakeAll ();
  if (yield) {
    QThread::yieldCurrentThread();
  }
}

} // namespace
