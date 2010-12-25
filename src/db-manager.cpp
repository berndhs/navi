#include "db-manager.h"

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
 *****************************************************************/

#include "deliberate.h"

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileDialog>
#include <QSqlQuery>
#include <QSqlRecord>
#include <QSqlError>
#include <QDateTime>
#include <QApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QTimer>
#include <QDebug>

using namespace deliberate;

namespace navi
{

DbManager::DbManager (QObject *parent)
  :QObject (parent),
   dbRunning (false)
{
}

void
DbManager::Connect ()
{
 
}


void
DbManager::Start ()
{
  QString dataDir = QDesktopServices::storageLocation
                    (QDesktopServices::DataLocation);
  QString geoBaseName = dataDir + QDir::separator() 
                         + QString ("geobase.sql");
  geoBaseName = Settings().simpleValue ("database/geobase",geoBaseName)
                                    .toString();
  Settings().setSimpleValue ("database/geobase",geoBaseName);
  

  StartDB (geoBase, "geoBaseCon", geoBaseName);

  QStringList  eventElements;
  eventElements << "nodes"
                << "ways"
                << "nodetags"
                << "waytags"
                << "waynodes"
                << "nodeparcels"
                << "wayparcels"
                << "relations"
                << "relationparts"
                << "relationtags";

  CheckDBComplete (geoBase, eventElements);

  dbRunning = true;
}

void
DbManager::Stop ()
{
  if (dbRunning) {
    dbRunning = false;
    geoBase.close ();
  }
}

void
DbManager::StartDB (QSqlDatabase & db,
                    const QString & conName, 
                    const QString & dbFilename)
{
  db = QSqlDatabase::addDatabase ("QSQLITE", conName);
  CheckFileExists (dbFilename);
  db.setDatabaseName (dbFilename);
  bool ok = db.open ();
qDebug () << " open db " << ok 
          << " dbname " << db.databaseName ()
          << " file " << dbFilename;
}

void
DbManager::CheckFileExists (const QString & filename)
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
DbManager::CheckDBComplete (QSqlDatabase & db,
                            const QStringList & elements)
{
  QString eltName, kind;
qDebug () << " checking DB for elements " << elements;
  for (int e=0; e<elements.size(); e++) {
    eltName = elements.at(e);
    kind = ElementType (db, eltName).toUpper();
qDebug () << " element " << eltName << " is kind " << kind;
    if (kind != "TABLE" && kind != "INDEX") {
      MakeElement (db, eltName);
    }
  }
}

QString
DbManager::ElementType (QSqlDatabase & db, const QString & name)
{
  QSqlQuery query (db);
  QString pat ("select * from main.sqlite_master where name=\"%1\"");
  QString cmd = pat.arg (name);
  bool ok = query.exec (cmd);
  if (ok && query.next()) {
    QString tipo = query.value(0).toString();
    return tipo;
  }
  return QString ("none");
}



void
DbManager::MakeElement (QSqlDatabase & db, const QString & element)
{
  QString filename (QString (":/schemas/%1.sql").arg (element));
  QFile  schemafile (filename);
  schemafile.open (QFile::ReadOnly);
  QByteArray  createcommands = schemafile.readAll ();
  schemafile.close ();
  QString cmd (createcommands);
  QSqlQuery query (db);
  query.prepare (cmd);
  bool ok = query.exec ();
qDebug () << " tried " << ok << " to create element with " 
          << query.executedQuery ();
}

void
DbManager::WriteNodeTag (const QString & nodeId, 
                     const QString & key,
                     const QString & value)
{
  WriteTag ("node",nodeId, key, value);
}

void
DbManager::WriteWayTag (const QString & wayId, 
                     const QString & key,
                     const QString & value)
{
  WriteTag ("way",wayId, key, value);
}

void
DbManager::WriteRelationTag (const QString & relId,
                     const QString & key,
                     const QString & value)
{
  WriteTag ("relation", relId, key, value);
}

void
DbManager::WriteTag (const QString & type,
                     const QString & id,
                     const QString & key,
                     const QString & value)
{
  QString cmd ("insert or replace into %1tags "
               " (%1id, key, value) "
               " VALUES (?, ?, ?)");
  QSqlQuery insert (geoBase);
  insert.prepare (cmd.arg (type));
  insert.bindValue (0,QVariant(id));
  insert.bindValue (1,QVariant(key));
  insert.bindValue (2,QVariant(value));
  insert.exec ();
}

void
DbManager::WriteRelationMember (const QString & relId,
                                const QString & type,
                                const QString & ref)
{
  QString cmd ("insert or replace into relationparts "
               "  (relationid, othertype, otherid) "
               " VALUES (?, ?, ?) ");
  QSqlQuery insert (geoBase);
  insert.prepare (cmd);
  insert.bindValue (0,QVariant (relId));
  insert.bindValue (1,QVariant (type));
  insert.bindValue (2,QVariant (ref));
  insert.exec ();
}

bool
DbManager::GetNodeTag (const QString & nodeId,
                      const QString & tagKey,
                            QString & tagValue)
{
  return GetTag ("way",nodeId,tagKey,tagValue);
}

bool
DbManager::GetWayTag (const QString & wayId,
                      const QString & tagKey,
                            QString & tagValue)
{
  return GetTag ("way",wayId,tagKey,tagValue);
}


bool
DbManager::GetTag (const QString & type,
                   const QString & id,
                   const QString & key,
                         QString & value)
{
  QString cmd ("select value from %1tags where %1id=\"%2\" "
                " AND key=\"%3\"");
  QSqlQuery select (geoBase);
  bool ok = select.exec (cmd.arg(type).arg(id).arg(key));
  if (ok && select.next()) {
    value = select.value(0).toString();
    return true;
  }
  return false;
}

bool
DbManager::GetNodeTags (const QString & nodeId,
                              QList <QPair <QString, QString> > & tagList)
{
  return GetTags ("node", nodeId, tagList);
}

bool
DbManager::GetWayTags (const QString & wayId,
                              QList <QPair <QString, QString> > & tagList)
{
  return GetTags ("way", wayId, tagList);
}

bool
DbManager::GetRelationTags (const QString & relId,
                              QList <QPair <QString, QString> > & tagList)
{
  return GetTags ("relation", relId, tagList);
}

bool
DbManager::GetTags (const QString & type,
                    const QString & id,
                          QList <QPair<QString, QString> > & list)
{
  QString cmd ("select key,value from %1tags where %1id=\"%2\"");
  QSqlQuery  select (geoBase);
  bool ok = select.exec (cmd.arg (type).arg (id));
  if (!ok) {
    return false;
  }
  list.clear ();
  while (select.next ()) {
    QString key = select.value(0).toString();
    QString value = select.value(1).toString();
    QPair <QString,QString> entry (key,value);
    list.append (entry);
  }
  return true;
}

bool
DbManager::GetRelationMembers (const QString & relId,
                              const QString & type,
                              QStringList & refList)
{
  QString cmd ("select otherid from relationparts "
               " where relationid =\"%1\""
               " AND othertype =\"%2\"");
  QSqlQuery select (geoBase);
  QString realCmd = cmd.arg(relId).arg(type);
  bool ok = select.exec (realCmd);
  if (!ok) {
    return false;
  }
  refList.clear ();
  while (select.next()) {
    refList.append (select.value(0).toString());
  }
  return true;
}

void
DbManager::GetRelations (const QString & otherId, 
                            const QString & type,
                            QStringList & relIdList)
{
  QString cmd ("select relationid from relationparts "
               " where othertype=\"%1\" and otherid = \"%2\"");
  QSqlQuery select (geoBase);
  QString realCmd = cmd.arg(type).arg(otherId);
  bool ok = select.exec (realCmd);
  qDebug () << " query cmd was " << realCmd;
  qDebug () << " query " << ok << select.executedQuery ();
  qDebug () << "      last error " << select.lastError().text();
  relIdList.clear ();
  if (!ok) {
    return;
  }
  while (select.next ()) {
    relIdList.append (select.value(0).toString());
  }
}

void
DbManager::WriteNodeParcel (const QString & nodeId, 
                            quint64 parcelIndex)
{
  WriteParcel ("node",nodeId, parcelIndex);
}

void
DbManager::WriteWayParcel (const QString & wayId,
                           quint64 parcelIndex)
{
  WriteParcel ("way", wayId, parcelIndex);
}

void
DbManager::WriteParcel (const QString & type,
                        const QString & id,
                        quint64 parcelIndex)
{
  QString cmd ("insert or replace into %1parcels "
               " (%1id, parcelid) "
               " VALUES (?, ?)");
  QSqlQuery insert (geoBase);
  insert.prepare (cmd.arg (type));
  insert.bindValue (0,QVariant (id));
  insert.bindValue (1,QVariant (parcelIndex));
  bool ok = insert.exec ();
  qDebug () << " write parcel " << ok << insert.executedQuery();
}

void
DbManager::WriteNode (const QString & nodeId,
                            double lat,
                            double lon)
{
  QString cmd ("insert or replace into nodes "
               " (nodeid, lat, lon) "
               " VALUES (?, ?, ?) ");
  QSqlQuery insert (geoBase);
  insert.prepare (cmd);
  insert.bindValue (0, QVariant(nodeId));
  insert.bindValue (1, QVariant(lat));
  insert.bindValue (2, QVariant(lon));
  bool ok = insert.exec ();
  qDebug () << " query " << ok << insert.executedQuery ();
}

void
DbManager::WriteWay (const QString & wayId)
{
  QString cmd ("insert or replace into ways "
               " (wayid) "
               " VALUES (?) ");
  QSqlQuery insert (geoBase);
  insert.prepare (cmd);
  insert.bindValue (0, QVariant(wayId));
  bool ok = insert.exec ();
  qDebug () << " query " << ok << insert.executedQuery ();
}

void
DbManager::WriteRelation (const QString & relId)
{
  QString cmd ("insert or replace into relations "
               " (relationid) "
               " VALUES (?) ");
  QSqlQuery insert (geoBase);
  insert.prepare (cmd);
  insert.bindValue (0, QVariant(relId));
  bool ok = insert.exec ();
  qDebug () << " query " << ok << insert.executedQuery ();
}

void
DbManager::WriteWayNode (const QString & wayId,
                         const QString & nodeId)
{
  QString cmd ("insert or replace into waynodes "
               " (wayid, nodeid) "
               " VALUES (?, ?) ");
  QSqlQuery insert (geoBase);
  insert.prepare (cmd);
  insert.bindValue (0, QVariant (wayId));
  insert.bindValue (1, QVariant (nodeId));
  bool ok = insert.exec ();
  qDebug () << " query " << ok << insert.executedQuery ();
}

bool
DbManager::GetNode (const QString & nodeId,
                    double & lat,
                    double & lon)
{
  QString cmd ("select lat, lon from nodes where nodeid =\"%1\"");
  QSqlQuery select (geoBase);
  bool ok = select.exec (cmd.arg (nodeId));
  if (ok && select.next()) {
    lat = select.value (0).toDouble();
    lon = select.value (1).toDouble();
  }
  return ok;
}

bool
DbManager::HaveWay (const QString & wayId)
{
  QString cmd ("select count(wayid) from ways where wayid = \"%1\"");
  QSqlQuery select (geoBase);
  bool ok = select.exec (cmd.arg (wayId));
  if (ok) {
    int count = select.value(0).toInt();
    return count > 0;
  }
  return false;
}

bool
DbManager::HaveRelation (const QString & relId)
{
  QString cmd ("select count(relationid) from relations "
              " where relationid = \"%1\"");
  QSqlQuery select (geoBase);
  bool ok = select.exec (cmd.arg (relId));
  if (ok) {
    int count = select.value(0).toInt();
    return count > 0;
  }
  return false;
}

bool
DbManager::GetWayNodes (const QString & wayId,
                        QStringList & nodeIdList)
{
  QString cmd ("select nodeid from waynodes where wayid = \"%1\"");
  QSqlQuery select (geoBase);
  bool ok = select.exec (cmd.arg (wayId));
  if (!ok) {
    return false;
  }
  nodeIdList.clear ();
  while (select.next()) {
    QString nodeId (select.value(0).toString());
    nodeIdList.append (nodeId);
  }
  return true;
}

bool
DbManager::GetNodes (quint64 parcelIndex,
                    QStringList & nodeIdList)
{
  return GetItems (parcelIndex,"node",nodeIdList);
}

bool
DbManager::GetWays (quint64 parcelIndex,
                    QStringList & wayIdList)
{
  return GetItems (parcelIndex,"way",wayIdList);
}

bool
DbManager::GetItems (quint64 parcelIndex,
                    const QString & type,
                    QStringList & idList)
{
  QString cmd ("select %1id from %1parcels where parcelid=%2");
  QSqlQuery select (geoBase);
  bool ok = select.exec (cmd.arg(type).arg (parcelIndex));
  if (!ok) {
    return false;
  }
  idList.clear ();
  while (select.next()) {
    idList.append (select.value(0).toString());
  }
  return true;
}

void
DbManager::GetByTag (QStringList & idList,
                     const QString & tagKey,
                     const QString & tagValue,
                     const QString & type,
                           bool regularExp)
{
  idList.clear();
  QString cmd ("select %1id from %1tags where key=\"%2\" "
                  "and value %3 \"%4\"");
  QSqlQuery select (geoBase);
  QString compareOp (regularExp ? " GLOB " : "=");
  QString realCmd  = cmd .arg (type)
                             .arg (tagKey)
                             .arg (compareOp)
                             .arg (tagValue) ;
  bool ok = select.exec (realCmd);
qDebug () << "GetByTag query " << ok << select.executedQuery();
  qDebug () << " query cmd was " << realCmd;
  qDebug () << "      last error " << select.lastError().text();
  if (ok) {
    while (select.next()) {
      idList.append (select.value(0).toString());
    } 
  }
}

void
DbManager::GetNodesByLatLon (QStringList & nodeList,
                            double south, double west,
                            double north, double east)
{
  nodeList.clear ();
  QString cmd ("select nodeid from nodes where "
               " lat >= %1 AND lat <= %2 "
               " AND "
               " lon >= %3 AND lon <= %4 ");
  QSqlQuery select (geoBase);
  QString realCmd = cmd.arg (south) . arg (north)
                       .arg (west) . arg (east);
  bool ok = select.exec (realCmd);
  if (!ok) {
    return;
  }
  while (select.next ()) {
    nodeList.append (select.value (0).toString());
  }
  return;
}

void
DbManager::GetWaysByNode (QStringList & wayList,
                          const QString & nodeId)
{
  wayList.clear ();
  QString cmd ("select wayid from waynodes where nodeid=\"%1\"");
  QSqlQuery select (geoBase);
  bool ok = select.exec (cmd.arg(nodeId));
  if (!ok) {
    return;
  }
  while (select.next()) {
    wayList.append (select.value(0).toString());
  }
}

void
DbManager::GetRelationsByMember (QStringList & relIdList,
                                 const QString & memType,
                                 const QString & memId)
{
  relIdList.clear ();
  QString cmd ("select relationid from relationparts "
               " where othertype=\"%1\" and otherid = \"%2\"");
  QSqlQuery select (geoBase);
  bool ok = select.exec (cmd.arg(memType).arg(memId));
  if (!ok) {
    return;
  }
  while (select.next()) {
    relIdList.append (select.value(0).toString());
  }
}

void
DbManager::StartTransaction ()
{
  geoBase.transaction ();
}

void
DbManager::CommitTransaction ()
{
  geoBase.commit ();
}


} // namespace

