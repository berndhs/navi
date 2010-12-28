#include "sqlite-runner.h"

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

namespace deliberate
{

SqliteRunner::SqliteRunner ()
  :doQuit (false),
   nextRequest (1234)
{
  wakeTimer = new QTimer (this);
  connect (wakeTimer, SIGNAL (timeout()), this, SLOT (Wake()));
}

SqliteRunner::~SqliteRunner ()
{
  Stop ();
  dbaseMap.clear ();
  queryMap.clear ();
  requestList.clear ();
}

void
SqliteRunner::CloseAll ()
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
SqliteRunner::CloseDB (int openId)
{
  bool ok (false);
  bool doemit (false);
  if (dbaseMap.contains (openId)) {
    QSqlDatabase * db = dbaseMap[openId];
    if (db) {
      db->close();
      delete db;
      ok = true;
    }
    doemit = true;
    dbaseMap.remove (openId);
  }
  return doemit;
}

void
SqliteRunner::Start (int priority)
{
  QThreadPool::globalInstance()->start (this, priority);
  wakeTimer->start (2000);
}

void
SqliteRunner::Stop ()
{
  doQuit = true;
  idleWait.wakeAll();
}

void
SqliteRunner::Wake ()
{
  idleWait.wakeAll ();
}

void
SqliteRunner::run ()
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
SqliteRunner::DoWork ()
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
  case Req_Query:
    DoQuery (req);
    break;
  case Req_SelectExec:
    DoSelectExec (req, false);
    break;
  case Req_SelectExecString:
    DoSelectExec (req, true);
    break;
  case Req_SelectPrepare:
    DoSelectPrepare (req);
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
SqliteRunner::SendSignals ()
{
  while (!signalList.isEmpty()) {
    SignalStruct sig = signalList.takeFirst ();
    switch (sig.type) {
    case Sig_Op:
      emit DoneOp (sig.value0, sig.okFlag, sig.value1);
      break;
    case Sig_Open:
      emit DoneOpen (sig.value0, sig.okFlag);
      break;
    case Sig_Close:
      emit DoneClose (sig.value0);
      break;
    case Sig_Result:
      emit ResultsReady (sig.value0, sig.okFlag, sig.results);
      break;
    default:
      break;
    }
  }
}

int
SqliteRunner::RequestOpen (const QString & db)
{
  RequestStruct req (Req_Open);
  req.data = db;
  requestLock.lock ();
  req.requestId = nextRequest++;
  req.openId = req.requestId;
  requestList.append (req);
  requestLock.unlock ();
  idleWait.wakeAll ();
  return req.openId;
}

int
SqliteRunner::RequestClose (int openId)
{
  RequestStruct req (Req_Close);
  req.openId = openId;
  requestLock.lock ();
  req.requestId = nextRequest++;
  requestList.append (req);
  requestLock.unlock ();
  idleWait.wakeAll ();
  return req.requestId;
}

int
SqliteRunner::RequestQuery (int openId, const QString & cmd)
{
  return QueueQuery (openId, Req_Query, 0, cmd);
}

int
SqliteRunner::RequestSelectExec (int openId,
                                 int numValues,
                                 const QString & cmd)
{
qDebug () << "RequestSelectExec " << cmd;
  return QueueQuery (openId, Req_SelectExecString, numValues, cmd);
}

int
SqliteRunner::RequestSelectExec (int prepareId)
{
  RequestStruct req (Req_SelectExec);
  req.queryId = prepareId;
  requestLock.lock ();
  req.requestId = nextRequest++;
  requestList.append (req);
  requestLock.unlock ();
  idleWait.wakeAll ();
  return req.requestId;
}

int
SqliteRunner::RequestSelectPrepare (int openId,
                                  int numValues,
                                  const QString & cmd)
{
  return QueueQuery (openId, Req_SelectPrepare, numValues, cmd);
}

int
SqliteRunner::RequestBind (int prepareId,
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
  return req.requestId;
}

int
SqliteRunner::QueueQuery (int openId,
                          RequestType type,
                                 int  numValues,
                                const QString & cmd)
{
  RequestStruct req (type);
  bool ok (false);
qDebug () << " Queue Query " << cmd;
  requestLock.lock ();
qDebug () << "    have lock ";
  if (dbaseMap.contains (openId)) {
    QSqlQuery * query = new QSqlQuery (*(dbaseMap[openId]));
    req.requestId = nextRequest++;
    queryMap[req.requestId] = query;
    req.queryId = req.requestId;
    req.openId = openId;
    req.numValues = numValues;
    req.data = cmd;
    ok = true;
  }
  int retval (-1);
  if (ok) {
    requestList.append (req);
    retval = req.queryId;
  } else {
    retval = -1;
  }
  requestLock.unlock ();
  idleWait.wakeAll ();
qDebug () << " unlocked ";
  return retval;
}

void
SqliteRunner::DoOpen (RequestStruct & req)
{
qDebug () << " DoOpen ";
  QSqlDatabase *db = new QSqlDatabase ;
  (*db) = QSqlDatabase::addDatabase ("QSQLITE");
  db->setDatabaseName (req.data);
qDebug () << " db name " << req.data;
qDebug () << " db ptr " << db;
  bool ok = db->open ();
  if (ok) {
    dbaseMap[req.openId] = db;
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
SqliteRunner::DoClose (RequestStruct &req)
{
  if (CloseDB (req.openId)) {
    SignalStruct sig (Sig_Close);
    sig.value0 = req.openId;
    signalList.append (sig);
  }
}

void
SqliteRunner::DoQuery (RequestStruct & req)
{
  SignalStruct sig (Sig_Op);
  sig.value0 = req.queryId;
  if (queryMap.contains (req.queryId)) {
    QSqlQuery * qry = queryMap[req.queryId];
    if (!qry) {
      return;     // corrupted map
    }
    bool ok = qry->exec (req.data);
    sig.okFlag = ok;
    sig.value1 = qry->numRowsAffected ();
    queryMap.remove (req.queryId);
    delete qry;
  } else {
    sig.okFlag = false;
  }
  signalList.append (sig);
}

void
SqliteRunner::DoSelectExec (RequestStruct & req, bool useString)
{
  SignalStruct sig (Sig_Result);
  sig.value0 = req.queryId;
  if (queryMap.contains (req.queryId)) {
    QSqlQuery * qry = queryMap[req.queryId];
    if (!qry) {
      return;     // corrupted map
    }
    bool ok;
qDebug () << " try query " << req.data;
    if (useString) {
      ok = qry->exec (req.data);
    } else {
      ok = qry->exec ();
    }
    sig.okFlag = ok;
    BuildResults (sig.results, req.numValues, *qry);
  } else {
    sig.okFlag = false;
  }
  signalList.append (sig);
}

void
SqliteRunner::DoSelectPrepare (RequestStruct & req)
{
  if (queryMap.contains (req.queryId)) {
    QSqlQuery * qry = queryMap[req.queryId];
    if (!qry) {
      return;     // corrupted map
    }
    qry->prepare (req.data);
  }
}

void
SqliteRunner::DoBind (RequestStruct & req)
{
  if (queryMap.contains (req.queryId)) {
    QSqlQuery * qry = queryMap[req.queryId];
    if (!qry) {
      return;     // corrupted map
    }
    qry->bindValue (req.bindIndex, req.bindValue, req.bindType);
  }
}

void
SqliteRunner::BuildResults (QVariant & results, 
                            int numColumns,
                            QSqlQuery & query)
{
  int row(0);
  QVariantMap resultMap;
  while (query.next()) {
    QVariantMap rowData;
    for (int col=0; col<numColumns; col++) {
      rowData[QString::number(col)] = query.value(col);
    }
    resultMap[QString::number(row)] = rowData;
    row++;
  }
  resultMap ["rows"] = QVariant (row);
  resultMap ["columns"] = QVariant (numColumns);
  results = resultMap;
}

} // namespace
