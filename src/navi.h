#ifndef NAVI_H
#define NAVI_H

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
#include "ui_navi.h"
#include "config-edit.h"
#include "helpview.h"

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QDomNode>

class QApplication;

using namespace deliberate;

namespace navi 
{

class Navi : public QMainWindow
{
Q_OBJECT

public:

  Navi (QWidget *parent=0);

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

private:

  void Connect ();
  void CloseCleanup ();
  void ShowWay (const QDomNode & node);

  bool             initDone;
  QApplication    *app;
  Ui_NaviMain    mainUi;
 
  ConfigEdit       configEdit;
  QStringList      configMessages;

  deliberate::HelpView        *helpView;
  bool             runAgain;

  QNetworkAccessManager   network;
  QNetworkReply          *reply;

};

} // namespace

#endif
