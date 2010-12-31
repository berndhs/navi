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
  int AskRangeNodes (double south, double west, 
                      double north, double east);
  int AskLatLon (const QString & nodeId);
  int AskNodeTagList (const QString & nodeId);

private slots:

  void CatchOpen (SqlRunDatabase* db, bool ok);
  void CatchClose (SqlRunDatabase *db);
  void CatchFinished (SqlRunQuery *query, bool ok);

signals:

  void HaveRangeNodes (int requestId, const QStringList & nodeList);
  void HaveLatLon (int requestId, double lat, double lon);
  void HaveTagList (int requestId, const TagList & tagList);


private:

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
    Query_AskTagList
  };

  struct QueryState {
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
