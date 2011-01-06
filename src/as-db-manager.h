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

#include "sql-runner.h"
#include "navi-types.h"

using namespace deliberate;

namespace navi
{
class AsDbManager : public QObject 
{
Q_OBJECT
public:

  AsDbManager (QObject *parent=0);
  ~AsDbManager ();

  void Start ();
  void Stop ();

  int PendingRequestCount ();

  void StartTransaction ();
  void CommitTransaction ();

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
  int SetRange (QString & tablePrefix, double south, double west, 
                      double north, double east);
  int GetRangeWays (const QString & tablePrefix, double south, double west, 
                      double north, double east);
  void DropTemp (const QString & dbName);
  int AskRangeNodes (double south, double west, 
                      double north, double east);
  int AskWaysByNode (const QString & nodeId);
  int AskWaysByTag (const QString & key, const QString & value, 
                    bool regular=false);
  int AskLatLon (const QString & nodeId);
  int AskNodeTagList (const QString & nodeId);
  int AskNodes (const QString & tablePrefix);
  int AskWays (const QString & tablePrefix);
  int AskRelations (const QString & tablePrefix);
  int AskNodeTags (const QString & tablePrefix);
  int AskWayTags (const QString & tablePrefix);
 
  int SetMark ();

private slots:

  void CatchOpen (SqlRunDatabase* db, bool ok);
  void CatchClose (SqlRunDatabase *db);
  void CatchFinished (SqlRunQuery *query, bool ok);
  void CatchMark (int markId, bool ok);

signals:

  void HaveRangeNodes (int requestId, const NaviNodeList & nodeList);
  void HaveLatLon (int requestId, double lat, double lon);
  void HaveTagList (int requestId, const TagList & tagList);
  void HaveWayList (int requestId, const QStringList & wayList);
  void HaveWayTurnList (int requestId, const WayTurnList & wayTurnList);
  void HaveRangeNodeTags (int requestId, const TagRecordList & tagList);
  void HaveTemp (int requestId, int ok);
  void MarkReached (int markId);


private:

  void WriteTag (const QString & type,
                 const QString & id,
                    const QString & key,
                    const QString & value);
  void WriteParcel (const QString & type,
                    const QString & id,
                    quint64 parcelIndex);
  void Connect ();
  SqlRunDatabase * StartDB (const QString & dbname);
  void CheckFileExists (const QString & filename);
  void CheckDBComplete (SqlRunDatabase * db, 
                        const QStringList & elements);
  void ContinueCheck (SqlRunDatabase * db);
  void AskElementType (SqlRunDatabase * db, const QString & eltName);
  void CheckElementType (SqlRunQuery *query, bool ok);
  void ReturnRangeNodes (SqlRunQuery *query, bool ok);
  void ReturnLatLon (SqlRunQuery *query, bool ok);
  void ReturnTagList (SqlRunQuery *query, bool ok);
  void ReturnWayList (SqlRunQuery *query, bool ok);
  void ReturnWayTurnList (SqlRunQuery *query, bool ok);
  void ReturnRangeNodeTags (SqlRunQuery *query, bool ok);
  void ReturnTemp (SqlRunQuery *query, bool ok);
  void MakeElement (SqlRunDatabase * db, const QString & elementName);

  struct DbState {
    bool    open;
    bool    checkInProgress;
    QString name;
  };

  enum QueryType {
    Query_None = 0,
    Query_IgnoreResult = 1,
    Query_AskElement,
    Query_AskRangeNodes,
    Query_AskLatLon,
    Query_AskTagList,
    Query_AskWayList,
    Query_AskWayTurnList,
    Query_RangeNodeTags,
    Query_CreateTemp
  };

  struct QueryState {
    QueryState () : finished (false), db(0) {}
    QueryState (int id, QueryType t, SqlRunDatabase *rdb = 0)
      :finished (false),
       reqId (id),
       type (t),
       db (rdb)
      {}
    bool            finished;
    int             reqId;
    QueryType       type;
    SqlRunDatabase *db;
    QVariant        data;
  };

  typedef QMap <SqlRunDatabase*, DbState>      DbMapType;
  typedef QMap <SqlRunQuery*, QueryState>   QueryMapType;

  DbMapType       dbMap;
  QueryMapType    queryMap;

  QMap <SqlRunDatabase*, QStringList>  dbCheckList;

  SqlRunner       *runner;
  SqlRunDatabase  *geoBase;
  int    nextRequest;
};

} // namespace

#endif
