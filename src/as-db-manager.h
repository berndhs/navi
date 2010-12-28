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

private:

  SqliteRunner  runner;
};

} // namespace

#endif
