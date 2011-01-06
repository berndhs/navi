#include "navi-types.h"

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

namespace navi
{
double DegreeResoluton (100000.0);

quint64
ParcelIndex (double lat, double lon)
{
  qint32 ilat = qRound (lat * 100000.0);
  qint32 ilon = qRound (lon * 100000.0);
  return (qint64(ilat) << 32) | ilon;
}

WayTurn::WayTurn ()
{
}

WayTurn::WayTurn (const QString & wayId, const QString & nodeId,
                  int seq, 
                  double lat, double lon)
  :mWay (wayId),
   mNode (nodeId),
   mSeq (seq),
   mLat (lat),
   mLon (lon)
{
}

WayTurn::WayTurn (const WayTurn & other)
  :mWay (other.mWay),
   mNode (other.mNode),
   mSeq (other.mSeq),
   mLat (other.mLat),
   mLon (other.mLon)
{
}

WayTurn &
WayTurn::operator = (const WayTurn & other)
{
  if (&other != this) {
    mWay = other.mWay;
    mNode = other.mNode;
    mSeq = other.mSeq;
    mLat = other.mLat;
    mLon = other.mLon;
  }
  return *this;
}

QString
WayTurn::WayId () const
{
  return mWay;
}

QString
WayTurn::NodeId () const
{
  return mNode;
}

int
WayTurn::Seq () const
{
  return mSeq;
}

double
WayTurn::Lat () const
{
  return mLat;
}

double
WayTurn::Lon () const
{
  return mLon;
}

void
WayTurn::SetWayId (const QString & id)
{
  mWay = id;
}

void
WayTurn::SetNodeId (const QString & nid)
{
  mNode = nid;
}

void
WayTurn::SetSeq (int s)
{
  mSeq = s;
}

void
WayTurn::SetLatLon (double lt, double ln)
{
  mLat = lt;
  mLon = ln;
}


} // namespace

