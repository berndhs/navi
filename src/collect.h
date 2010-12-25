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
#include <QTime>

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
  bool  Run (const QStringList & files);
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
  void ReadNextXML ();
  void SaveSql ();
  void SendNext ();
  
  void SaveSequence ();
  void SaveNodesSql ();
  void SaveWaysSql ();
  void SaveRelationsSql ();

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

  enum Save_Stage {
    Stage_None = 0,
    Stage_Nodes = 1,
    Stage_Ways = 2,
    Stage_Relations = 3,
    Stage_Final = 4,
    Stage_Done
  };

  typedef QMap <QString, NaviNode>   NodeMapType;
  typedef QPair <QString, QString>   AttrType;
  typedef QList <AttrType>           AttrList;

  void Connect ();
  void CloseCleanup ();
  void SetDefaults ();
  void ContinueSequence ();
  void SendRequest (double west, double east, double south, double north);
  void ProcessNodes (QDomDocument & doc);
  void ProcessWays (QDomDocument & doc);
  void ProcessRelations (QDomDocument & doc);
  void ProcessWay (const QDomNode & node);
  void ProcessRelation (const QDomNode & node);
  void ProcessData (QByteArray & data);
  void BuildWayParcels (const QString & wayId, 
                        const QStringList & nodeIdList);
  void ShowProgress ();
  void LogStatus (const QString & msg);

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
  Save_Stage                   saveStage;
  QString                      lastUrl;
  QTime                        mClock;

  QByteArray                   responseBytes;
  NodeMapType                  nodeMap;
  QMap <QString, AttrList>     wayAttrMap;
  QMap <QString, AttrList>     nodeAttrMap;
  QMap <QString, AttrList>     relationAttrMap;
  QMap <QString, AttrList>     relationMembers;
  QMap <QString, QStringList>  wayNodes;

  QStringList                  inputFiles;
  QString                      currentFile;
  bool                         readingXML;
};

} // namespace

#endif
