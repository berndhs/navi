#ifndef AS_DB_MANAGER_H
#define AS_DB_MANAGER_H


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

#include "sqlite-runner.h"

using namespace deliberate;

namespace navi
{
class AsDbManager : public QObject 
{
Q_OBJECT

  AsDbManager (QObject *parent=0);
  ~AsDbManager ();

  void Start ();
  void Stop ();

  void WriteNode (const QString & nodeId,
                        double  lat,
                        double  lon);

  void WriteWay (const QString & wayId);
  void WriteRelation (const QString & relId);
  void WriteWayNode (const QString & wayId,
                     const QString & nodeId);
  void WriteNodeTag (const QString & nodeId, 
                     const QString & key,
                     const QString & value);
  void WriteWayTag (const QString & wayId,
                    const QString & key,
                    const QString & value);
  void WriteRelationTag (const QString & relId,
                         const QString & key,
                         const QString & value);
  void WriteRelationMember (const QString & relId,
                            const QString & type,
                            const QString & ref);
  void WriteNodeParcel (const QString & nodeId, 
                   quint64 parcelIndex);
  void WriteWayParcel (const QString & wayId,
                  quint64 parcelIndex); 

private slots:

  void CatchOpen (int dbHandle, bool ok);
  void CatchClose (int dbHandle);
  void CatchOp (int queryHandle, bool ok, int numRowsAffected);
  void CatchResults (int queryHandle, bool ok, const QVariant & results);

signals:

  void DoneStartDB (int handle, const QString & name);
  void DoneCheckComplete (int handle);

private:

  void Connect ();
  void StartDB (int & handle, const QString & dbname);
  void CheckFileExists (const QString & filename);
  void CheckDBComplete (int dbHandle, const QStringList & elements);
  void ContinueCheck (int dbHandle);
  void AskElementType (int dbHandle, const QString & eltName);
  void CheckElementType (int queryHandle, bool ok,
                         const QVariant & results);
  void MakeElement (int dbHandle, const QString & elementName);

  struct DbState {
    bool    open;
    bool    checkInProgress;
    QString name;
  };

  enum QueryType {
    Query_None = 0,
    Query_IgnoreResult = 1,
    Query_AskElement
  };

  struct QueryState {
    bool       finished;
    QueryType  type;
    int        dbHandle;
    QVariant   data;
  };

  typedef QMap <int, DbState>      DbMapType;
  typedef QMap <int, QueryState>   QueryMapType;

  DbMapType       dbMap;
  QueryMapType    queryMap;

  QMap <int, QStringList>  dbCheckList;

  SqliteRunner   *runner;
  int             geoBaseHandle;
};

} // namespace

#endif
