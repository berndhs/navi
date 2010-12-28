#ifndef NVROUTE_H
#define NVROUTE_H

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
#include <QMainWindow>
#include "ui_nvroute.h"
#include "config-edit.h"
#include "helpview.h"
#include "db-manager.h"
#include "navi-types.h"
#include "route-cell-menus.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDomNode>
#include <QMap>

class QApplication;
class QTreeWidgetItem;
class QAction;

using namespace deliberate;

namespace navi 
{

class NvRoute : public QMainWindow
{
Q_OBJECT

public:

  NvRoute (QWidget *parent=0);

  void  Init (QApplication &ap);
  bool  Run ();
  bool  Again ();

  void  AddConfigMessages (const QStringList & cm) 
           { configMessages.append (cm); }

  void closeEvent ( QCloseEvent *event);

private slots:

  void Quit ();
  void Restart ();
  void EditSettings ();
  void SetSettings ();
  void About ();
  void License ();
  void Exiting ();

  void LatLonButton ();
  void ParcelButton ();
  void FeatureButton ();
  void FindWays ();
  void FindRelations ();
  void FindThings ();
  void FindNodes ();
  void Picked (QTreeWidgetItem *item, int column);

private:

  enum CellType {
       Cell_NoType = 0,
       Cell_Node = 1,
       Cell_Way,
       Cell_Relation,
       Cell_Tag,
       Cell_LatLon,
       Cell_Header,
       Cell_Bad
  };


  void Connect ();
  void CloseCleanup ();
  void SetDefaults ();
  void ListWayDetails (const QString & wayId);
  void ListWayDetails (QTreeWidgetItem *item,
                       const QString & wayId);
  void ListNodeDetails (QTreeWidgetItem * item,
                        const QString & nodeId);
  void ListRelationDetails (QTreeWidgetItem * relItem,
                        const QString & nodeId);
  void ListWays ();
  void ListNodes ();
  void ListRelations ();
  void ListNodeRelations ();
  void FindParcel (quint64 parcel, int round);
  void CellMenuTop (QTreeWidgetItem *item,
                    int column);
  void CollectRelated (QTreeWidgetItem *item);
  QString CellTypeName (CellType type);

  bool             initDone;
  QApplication    *app;
  Ui_NvRouteMain   mainUi;
 
  ConfigEdit       configEdit;
  QStringList      configMessages;

  deliberate::HelpView        *helpView;
  bool             runAgain;

  DbManager                    db;

  QSet<QString>   nodeSet;
  QSet<QString>   waySet;
  QSet<QString>   relationSet;
  QStringList     wayList;
  quint64         parcelIndex;
  QList<quint64>  indexList;
  QTimer         *findTimer;

  QAction        *collectAction;
  RouteCellMenu  *cellMenu;

  QMap <CellType, QString > cellTypeName;

};

} // namespace

#endif
