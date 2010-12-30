#ifndef DELIBERATE_SQLITE_RUNNER_H
#define DELIBERATE_SQLITE_RUNNER_H


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


#include <QObject>
#include <QRunnable>
#include <QString>
#include <QStringList>
#include <QMutex>
#include <QWaitCondition>

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QMap>
#include <QList>
#include <QVariant>

class QTimer;

namespace deliberate
{

class SqlRunQuery;
class SqlRunDatabase;

class SqlRunner : public QObject, public QRunnable
{
Q_OBJECT
public:


  SqlRunner ();
  ~SqlRunner ();

  void Start (int priority=0);
  void Stop ();
  void run ();

  SqlRunDatabase  *openDatabase (const QString & filename);
  SqlRunQuery     *newQuery (SqlRunDatabase * db);

signals:

  void Finished (SqlRunQuery * query, bool ok);
  void Opened (SqlRunDatabase * db, bool ok);
  void Closed (SqlRunDatabase * db);

private slots:

  void Wake ();

private:

  enum RequestType {
       Req_None = 0,
       Req_Open = 1,
       Req_Close = 2,
       Req_ExecString = 3,
       Req_Exec = 4,
       Req_Prepare = 5,
       Req_Bind = 6,
       Req_DeallocQuery = 7,
       Req_Bad
  };

  enum SignalType {
       Sig_None = 0,
       Sig_Op = 1,
       Sig_Open,
       Sig_Close,
       Sig_Result,
       Sig_Bad
  };

  struct RequestStruct {
      RequestStruct (RequestType t=Req_None) : type(t){}
      RequestType      type;
      int              requestId;
      int              openId;
      int              queryId;
      int              bindIndex;
      QSql::ParamType  bindType;
      QString          data;
      QVariant         bindValue;
  };

  struct SignalStruct {
      SignalStruct (SignalType t=Sig_None) : type(t){}
      SignalType  type;
      bool        okFlag;
      int         value0;
      int         value1;
  };

  typedef QMap<int, SqlRunDatabase*>    DBaseMapType;
  typedef QMap<SqlRunDatabase*, int>    RevDBaseMapType;
  typedef QMap<int, SqlRunQuery*>       QueryMapType;

  void RequestOpen (int dbid);

  void RequestClose (int openId);

  void RequestExec (int queryHandle, 
                  const QString & cmd);

  void RequestExec (int queryHandle);

  void RequestPrepare (int queryHandle, 
                      const QString & cmd);

  void RequestBind (int queryHandle, 
                         int index,
                      const QVariant & value,  
                      QSql::ParamType paramType = QSql::In);

  void RequestDelete (int queryHandle);

  void QueueQuery (int openId,
                  RequestType type,
                  const QString & cmd);

  bool CloseDB (int openId);
  void CloseAll ();

  bool DoWork ();
  void SendSignals ();
 
  void DoOpen (RequestStruct & req);
  void DoClose (RequestStruct & req);
  void DoExec  (RequestStruct & req, bool useString);
  void DoPrepare (RequestStruct & req);
  void DoBind (RequestStruct & req);
  void DoDealloc (RequestStruct & req);

  void BuildResults (SqlRunQuery * query);

  void WakeAll (bool yield=false);

  SqlRunDatabase * database (int dbHandle);

  QMutex          requestLock;
  QMutex          doLock;
  QMutex          idleLock;
  QWaitCondition  idleWait;
  bool            doQuit;
  bool            doneQuit;

  QList <RequestStruct>  requestList;
  QList <SignalStruct>   signalList;

  DBaseMapType     dbaseMap;
  RevDBaseMapType  revDbaseMap;
  
  QueryMapType     queryMap;

  int             nextRequest;

  QTimer         *wakeTimer;

  friend class SqlRunDatabase;
  friend class SqlRunQuery;

};

} // namespace

#endif
