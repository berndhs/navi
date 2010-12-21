#ifndef COLLECT_H
#define COLLECT_H

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
#include "ui_collect.h"
#include "config-edit.h"
#include "helpview.h"
#include "db-manager.h"
#include "navi-types.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDomNode>
#include <QMap>

class QApplication;

using namespace deliberate;

namespace navi 
{

class Collect : public QMainWindow
{
Q_OBJECT

public:

  Collect (QWidget *parent=0);

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

  void ReadButton ();
  void HandleReply (QNetworkReply *reply);
  void SaveResponse ();
  void ReadXML ();
  void SaveSql ();
  void SendNext ();

private:

  class Highway {
  public:
    Highway ();
    void  clear ();

    QString    name;
    QString    kind;
    bool       ishighway;
    QMap <QString, QString>  attributes;
  };

  typedef QMap <QString, NaviNode>   NodeMapType;
  typedef QPair <QString, QString>   AttrType;
  typedef QList <AttrType>           AttrList;

  void Connect ();
  void CloseCleanup ();
  void SetDefaults ();
  void SendRequest (double west, double east, double south, double north);
  void ProcessWay (const QDomNode & node);
  void SaveNodesSql ();
  void SaveWaysSql ();
  void ProcessData (QByteArray & data);
  void BuildWayParcels (const QString & wayId, 
                        const QStringList & nodeIdList);
  void ShowProgress ();

  bool             initDone;
  QApplication    *app;
  Ui_CollectMain    mainUi;
 
  ConfigEdit       configEdit;
  QStringList      configMessages;

  deliberate::HelpView        *helpView;
  bool             runAgain;

  DbManager                    db;
  QNetworkAccessManager        network;
  QNetworkReply               *reply; 
  double                       southEnd;
  double                       minSouth;
  double                       eastEnd;
  double                       westEnd;
  double                       minWest;
  double                       northEnd;
  double                       latStep;
  double                       lonStep;
  bool                         useNetwork;
  bool                         autoGet;
  QString                      lastUrl;

  QByteArray                   responseBytes;
  NodeMapType                  nodeMap;
  QMap <QString, AttrList>     wayAttrMap;
  QMap <QString, QStringList>  wayNodes;
};

} // namespace

#endif
