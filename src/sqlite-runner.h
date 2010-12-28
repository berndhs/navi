#ifndef SQLITE_RUNNER_H
#define SQLITE_RUNNER_H


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

class SqliteRunner : public QObject, public QRunnable
{
Q_OBJECT
public:

  SqliteRunner ();
  ~SqliteRunner ();

  void Start (int priority=0);
  void Stop ();
  void run ();

  /** \brief RequestOpen 
    * @return  handle (openId) for the connection, or -1 on failure
    */
  int RequestOpen (const QString & db);

  /** \brief RequestClose
    * @return  same as openId
    */
  int RequestClose (int openId);

  /** \brief RequestQuery
    * @return handle for the return signal DoneOp
    */
  int RequestQuery (int openId, const QString & cmd);

  /** \brief RequestSelectExec
    * @return handle for the return signal ResultsReady
    */
  int RequestSelectExec (int openId, 
                         int numResults,
                     const QString & cmd);

  /** \brief RequestSelectExec
    * @return handle for the return signal ResultsReady
    */
  int RequestSelectExec (int prepareId);

  /** \brief RequestSelectPrepare
    * @return handle to be used in RequestBind
    */
  int RequestSelectPrepare (int openId, 
                         int numResults,
                      const QString & cmd);

  /** \brief RequestBind
    * @param prepareId the handle for the select from RequestSelectPrepare
    * @return bind request id
    */
  int RequestBind (int prepareId, 
                         int index,
                      const QVariant & value,  
                      QSql::ParamType paramType = QSql::In);

signals:

  void ResultsReady (int selectId, 
                     bool ok, 
                     const QVariant & resultRows);
  void DoneOp (int opId, bool ok, int rowsAffected);
  void DoneClose (int openId);
  void DoneOpen (int openId, bool ok);

private slots:

  void Wake ();

private:

  enum RequestType {
       Req_None = 0,
       Req_Open = 10,
       Req_Close = 20,
       Req_Query = 30,
       Req_SelectExecString = 40,
       Req_SelectExec = 41,
       Req_SelectPrepare = 50,
       Req_Bind = 60,
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

  class RequestStruct {
    public:
      RequestStruct (RequestType t=Req_None) : type(t){}
      RequestType      type;
      int              requestId;
      int              openId;
      int              queryId;
      int              numValues;
      int              bindIndex;
      QSql::ParamType  bindType;
      QString          data;
      QVariant         bindValue;
  };

  class SignalStruct {
    public:
      SignalStruct (SignalType t=Sig_None) : type(t){}
      SignalType  type;
      bool        okFlag;
      int         value0;
      int         value1;
      QVariant    results;
  };

  typedef QMap<int, QSqlDatabase*> DBaseMapType;
  typedef QMap<int, QSqlQuery*>    QueryMapType;

  int QueueQuery (int openId,
                  RequestType type,
                  int  numValues,
                  const QString & cmd);

  bool CloseDB (int openId);
  void CloseAll ();

  bool DoWork ();
  void SendSignals ();
 
  void DoOpen (RequestStruct & req);
  void DoClose (RequestStruct & req);
  void DoQuery (RequestStruct & req);
  void DoSelectExec  (RequestStruct & req, bool useString);
  void DoSelectPrepare (RequestStruct & req);
  void DoBind (RequestStruct & req);

  void BuildResults (QVariant & results,
                     int  numColumns,
                     QSqlQuery & query);

  QMutex          requestLock;
  QMutex          doLock;
  QMutex          idleLock;
  QWaitCondition  idleWait;
  bool            doQuit;

  QList <RequestStruct>  requestList;
  QList <SignalStruct>   signalList;

  DBaseMapType    dbaseMap;
  QueryMapType    queryMap;

  int             nextRequest;

  QTimer         *wakeTimer;

};

} // namespace

#endif
