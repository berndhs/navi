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

  QString Id () const { return id; }
  double  Lat () const { return lat; }
  double  Lon () const { return lon; }

  void SetLat (double l) { lat = l; }
  void SetLon (double l) { lon = l; }

private:

  QString id;
  double  lat;
  double  lon;

};

class TagRecord {
public:

  TagRecord () {}
  TagRecord (const QString & theId, 
             const QString & theKey, 
             const QString & theVal)
    :id (theId), key (theKey), value (theVal) {}
  TagRecord (const TagRecord & other)
    {
      id = other.id;
      key = other.key;
      value = other.value;
    }

  QString Id () const { return id; }
  QString Key () const { return key; }
  QString Value () const { return value; }

private:

  QString id;
  QString key;
  QString value;
};

typedef QPair <QString, QString>  TagItemType;
typedef QList <TagItemType>       TagList;
typedef QList <NaviNode>          NaviNodeList;
typedef QList <TagRecord>         TagRecordList;

} // namespace

#endif