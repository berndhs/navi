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

  void WriteNode (const QString & nodeid,
                        double  lat,
                        double  lon);
  void WriteWay (const QString & wayid);
  void WriteWayNode (const QString & wayid,
                     const QString & nodeid);
  void WriteNodeTag (const QString & nodeid, 
                     const QString & key,
                     const QString & value);
  void WriteWayTag (const QString & wayid,
                    const QString & key,
                    const QString & value);

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

  QSqlDatabase  geoBase;
  bool          dbRunning;

};

} // namespace

#endif
