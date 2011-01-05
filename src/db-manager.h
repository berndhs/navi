#ifndef DB_MANAGER_H
#define DB_MANAGER_H

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

#include <QString>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QObject>
#include <QPair>
#include <QList>

namespace navi
{

class DbManager : public QObject
{
Q_OBJECT

public:

  DbManager (QObject *parent=0);

  void Start ();
  void Stop ();

  void StartTransaction ();
  void CommitTransaction ();

  void WriteNode (const QString & nodeId,
                        double  lat,
                        double  lon);


  void WriteWayLoc (const QString & wayId,
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
  bool GetNode (const QString & nodeId, double & lat, double & lon);
  bool HaveWay (const QString & wayId);
  bool HaveRelation (const QString & relId);
  bool GetWayNodes (const QString & wayId,
                 QStringList & nodeIdList);
  bool GetNodes (quint64 parcelIndex,
                QStringList & nodeIdList);
  bool GetWays (quint64 parcelIndex,
                QStringList & wayIdList);
  bool GetNodeTag (const QString & nodeid,
                   const QString & tagKey,
                         QString & tagValue);
  bool GetNodeTags (const QString & nodeId,
                         QList<QPair <QString,QString> > & tagList);
  bool GetWayTag (const QString & wayId,
                  const QString & tagKey,
                        QString & tagValue);
  bool GetWayTags (const QString & wayId,
                        QList <QPair <QString, QString> > & tagList);
  bool GetRelationTags (const QString & relId,
                        QList <QPair <QString, QString> > & tagList);
  bool GetRelationMembers (const QString & relId,
                           const QString & type,
                           QStringList & refList);
  void GetByTag (QStringList & idList,
                 const QString & tagKey,
                 const QString & tagValue,
                 const QString & type,
                 bool regularExp = false);
  void GetNodesByLatLon (QStringList & nodeList,
                        double south, double west,
                        double north, double east);
  void GetWaysByNode (QStringList & wayList,
                      const QString & nodeId);
  void GetRelationsByMember (QStringList & relIdList,
                             const QString & memType,
                             const QString & memId);

public slots:


private slots:


private:

  void StartDB (QSqlDatabase & db,
                    const QString & conName, 
                    const QString & dbFilename);
  void CheckFileExists (const QString & filename);
  void CheckDBComplete (QSqlDatabase & db,
                        const QStringList & elements);
  QString ElementType (QSqlDatabase & db, const QString & name);
  void    MakeElement (QSqlDatabase & db, const QString & element);
  void Connect ();

  void WriteTag (const QString & type,
                 const QString & id,
                    const QString & key,
                    const QString & value);
  void WriteParcel (const QString & type,
                    const QString & id,
                    quint64 parcelIndex);
  bool GetItems (quint64 parcelIndex,
                 const QString & type,
                 QStringList & idList);
  bool GetTag (const QString & type,
               const QString & id, 
               const QString & key,
                     QString & value);
  bool GetTags (const QString & type,
                const QString & id,
                      QList<QPair <QString, QString> >  & list);

  QSqlDatabase  geoBase;
  int           geoBaseHandle;
  bool          dbRunning;

};

} // namespace

#endif
