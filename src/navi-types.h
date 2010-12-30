#ifndef NAVI_TYPES_H
#define NAVI_TYPES_H

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
#include <QPair>

namespace navi
{

typedef QPair <QString, QString>  TagItemType;
typedef QList <TagItemType>       TagList;

class NaviNode 
{
public:

  NaviNode ()
    :lat(0.0),lon (0.0) {}
  NaviNode (const QString & nodeId, double nodeLat, double nodeLon)
    :id (nodeId), lat (nodeLat), lon (nodeLon) {}
  NaviNode (const NaviNode &other)
    {
       id = other.id;
       lat = other.lat;
       lon = other.lon;
    }
  NaviNode & operator = (const NaviNode & other) 
    {
       if (&other != this) {
         id = other.id;
         lat = other.lat;
         lon = other.lon;
       }
       return *this;
    }

  QString Id () { return id; }
  double  Lat () { return lat; }
  double  Lon () { return lon; }

  void SetLat (double l) { lat = l; }
  void SetLon (double l) { lon = l; }

private:

  QString id;
  double  lat;
  double  lon;

};

} // namespace

#endif