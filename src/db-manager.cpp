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
  geoBaseName = Settings().value ("database/geobase",geoBaseName)
                                    .toString();
  Settings().setValue ("database/geobase",geoBaseName);
  

  StartDB (geoBase, "geoBaseCon", geoBaseName);

  QStringList  eventElements;
  eventElements << "nodes"
                << "ways"
                << "nodetags"
                << "waytags"
                << "waynodes";

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
DbManager::WriteNodeTag (const QString & nodeid, 
                     const QString & key,
                     const QString & value)
{
  WriteTag ("node",nodeid, key, value);
}

void
DbManager::WriteWayTag (const QString & wayid, 
                     const QString & key,
                     const QString & value)
{
  WriteTag ("way",wayid, key, value);
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
  bool ok = insert.exec ();
  qDebug () << " query " << ok << insert.executedQuery ();
}

void
DbManager::WriteNode (const QString & nodeid,
                            double lat,
                            double lon)
{
  QString cmd ("insert or replace into nodes "
               " (nodeid, lat, lon) "
               " VALUES (?, ?, ?) ");
  QSqlQuery insert (geoBase);
  insert.prepare (cmd);
  insert.bindValue (0, QVariant(nodeid));
  insert.bindValue (1, QVariant(lat));
  insert.bindValue (2, QVariant(lon));
  bool ok = insert.exec ();
  qDebug () << " query " << ok << insert.executedQuery ();
}

void
DbManager::WriteWay (const QString & wayid)
{
  QString cmd ("insert or replace into ways "
               " (wayid) "
               " VALUES (?) ");
  QSqlQuery insert (geoBase);
  insert.prepare (cmd);
  insert.bindValue (0, QVariant(wayid));
  bool ok = insert.exec ();
  qDebug () << " query " << ok << insert.executedQuery ();
}

void
DbManager::WriteWayNode (const QString & wayid,
                         const QString & nodeid)
{
  QString cmd ("insert or replace into waynodes "
               " (wayid, nodeid) "
               " VALUES (?, ?) ");
  QSqlQuery insert (geoBase);
  insert.prepare (cmd);
  insert.bindValue (0, QVariant (wayid));
  insert.bindValue (1, QVariant (nodeid));
  bool ok = insert.exec ();
  qDebug () << " query " << ok << insert.executedQuery ();
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

