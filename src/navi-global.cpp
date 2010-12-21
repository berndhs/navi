#include "navi-global.h"

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
#include <QDebug>

namespace navi
{

double Parcel::DegreeResolution (60.0 * 2.0);

quint64
Parcel::Index (double lat, double lon)
{
  quint64 ilat = qRound64 ((lat + 180.0) * DegreeResolution);
  quint64 ilon = qRound64 ((lon + 180.0) * DegreeResolution);
  quint64 result = (qint64(ilat) << 32) | qint64(ilon);
  return result;
}

void
Parcel::LatLon (quint64 parcelIndex, double & lat, double & lon)
{
  quint32 ilon = parcelIndex & Q_INT64_C(0xffffffff);
  quint32 ilat = ((parcelIndex & Q_INT64_C(0xffffffff00000000)) >> 32) ;
  lat = (double (ilat) / DegreeResolution) - 180.0;
  lon = (double (ilon) / DegreeResolution) - 180.0;
}

} // namespace

