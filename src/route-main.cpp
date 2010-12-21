
#include "main.h"

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


#include <QApplication>
#include "deliberate.h"
#include "version.h"
#include "cmdoptions.h"
#include "nvroute.h"


int
main (int argc, char *argv[])
{
  QCoreApplication::setOrganizationName ("BerndStramm");
  QCoreApplication::setOrganizationDomain ("bernd-stramm.com");
  QCoreApplication::setApplicationName ("navi");
  deliberate::ProgramVersion pv ("NvRoute");
  QCoreApplication::setApplicationVersion (pv.Version());
  deliberate::DSettings  settings;
  deliberate::InitSettings ();
  deliberate::SetSettings (settings);
  deliberate::Settings().SetPrefix ("nvroute_");
  settings.setValue ("program",pv.MyName());


  QStringList  configMessages;

  deliberate::CmdOptions  opts ("navi");
  opts.AddSoloOption ("debug","D",QObject::tr("show Debug log window"));
  opts.AddStringOption ("logdebug","L",QObject::tr("write Debug log to file"));

  deliberate::UseMyOwnMessageHandler ();

  bool optsOk = opts.Parse (argc, argv);
  if (!optsOk) {
    opts.Usage ();
    exit(1);
  }
  if (opts.WantHelp ()) {
    opts.Usage ();
    exit (0);
  }
  configMessages.append (QString("Built on %1 %2")
                         .arg (__DATE__).arg(__TIME__));
  configMessages.append (QObject::tr("Build with Qt %1").arg(QT_VERSION_STR));
  configMessages.append (QObject::tr("Running with Qt %1").arg(qVersion()));

  if (opts.WantVersion ()) {
    exit (0);
  }
  bool showDebug = opts.SeenOpt ("debug");
  int result;
  QApplication  app (argc, argv);

#if DELIBERATE_DEBUG
  deliberate::StartDebugLog (showDebug);
  bool logDebug = opts.SeenOpt ("logdebug");
  if (logDebug) {
    QString logfile ("/dev/null");
    opts.SetStringOpt ("logdebug",logfile);
    deliberate::StartFileLog (logfile);
  }
#endif
  
  navi::NvRoute   nvroute;

  app.setWindowIcon (nvroute.windowIcon());
  nvroute.Init (app);
  nvroute.AddConfigMessages (configMessages);

  nvroute.Run ();
  result = app.exec ();
  qDebug () << " QApplication exec finished " << result;
  return result;
}
