#include "asroute.h"
#include "deliberate.h"
#include "version.h"

#include <QMessageBox>
#include <QTimer>

using namespace deliberate;

namespace navi
{
AsRoute::AsRoute (QWidget *parent)
  :QMainWindow (parent),
   app (0),
   db (this),
   configEdit (this),
   helpView (0),
   cellMenu (0)
{
  mainUi.setupUi (this);
  mainUi.actionRestart->setEnabled (false);
  helpView = new HelpView (this);
  cellMenu = new RouteCellMenu (this);
  cellTypeName[Cell_NoType] = "NoType";
  cellTypeName[Cell_Node] = "Node";
  cellTypeName[Cell_Way] = "Way";
  cellTypeName[Cell_Relation] = "Relation";
  cellTypeName[Cell_Tag] = "Tag";
  cellTypeName[Cell_LatLon] = "LatLon";
  cellTypeName[Cell_Header] = "Header";
  cellTypeName[Cell_Bad] = "Bad";
  Connect ();
}

void
AsRoute::Init (QApplication & qapp)
{
  app = &qapp;
  QCoreApplication::setAttribute (Qt::AA_DontShowIconsInMenus, false);
  Settings().sync();
  db.Start ();
}

void
AsRoute::Run ()
{
  show ();
  qDebug () << " Start AsRoute";
  QSize defaultSize = size();
  QSize newsize = Settings().value ("sizes/main", defaultSize).toSize();
  resize (newsize);
  Settings().setValue ("sizes/main",newsize);
  show ();
  SetDefaults ();
}

void
AsRoute::closeEvent (QCloseEvent *event)
{
  Q_UNUSED (event)
  if (app) {
    app->quit();
  }
}


void
AsRoute::SetDefaults ()
{
  double south (0.0);
  double north (0.0);
  double east (0.0);
  double west (0.0);
  south = Settings().value ("defaults/south",south).toDouble();
  Settings().setValue ("defaults/south",south);
  north = Settings().value ("defaults/north",north).toDouble();
  Settings().setValue ("defaults/north",north);
  east = Settings().value ("defaults/east",east).toDouble();
  Settings().setValue ("defaults/east",east);
  west = Settings().value ("defaults/west",west).toDouble();
  Settings().setValue ("defaults/west",west);
  quint64 parcel (0);
  parcel = Settings().value ("defaults/parcel",parcel).toULongLong();
  Settings().setValue ("defaults/parcel",parcel);
  Settings().sync();
  mainUi.southValue->setValue (south);
  mainUi.northValue->setValue (north);
  mainUi.westValue->setValue (west);
  mainUi.eastValue->setValue (east);
  mainUi.parcelEdit->setText (QString::number(parcel));
}

void
AsRoute::Connect ()
{
  connect (mainUi.actionQuit, SIGNAL (triggered()), 
           this, SLOT (Quit()));
  connect (mainUi.actionSettings, SIGNAL (triggered()),
           this, SLOT (EditSettings()));
  connect (mainUi.actionAbout, SIGNAL (triggered()),
           this, SLOT (About ()));
  connect (mainUi.actionLicense, SIGNAL (triggered()),
           this, SLOT (License ()));
  connect (mainUi.latlonButton, SIGNAL (clicked()),
           this, SLOT (LatLonButton ()));
  connect (mainUi.parcelButton, SIGNAL (clicked()),
           this, SLOT (ParcelButton ()));
  connect (mainUi.featureButton, SIGNAL (clicked()),
           this, SLOT (FeatureButton ()));
  connect (mainUi.featureDisplay, SIGNAL (itemDoubleClicked (QTreeWidgetItem*,int)),
           this, SLOT (Picked (QTreeWidgetItem*, int)));
}

void
AsRoute::AddConfigMessages (const QStringList & argList)
{
  configMessages = argList;
}


void
AsRoute::Quit ()
{
  CloseCleanup ();
  if (app) {
    app->quit();
  }
}

void
AsRoute::CloseCleanup ()
{
  QSize currentSize = size();
  Settings().setValue ("sizes/main",currentSize);
  Settings().sync();
}

void
AsRoute::EditSettings ()
{
  configEdit.Exec ();
  SetSettings ();
}

void
AsRoute::SetSettings ()
{
  Settings().sync ();
}

void
AsRoute::About ()
{
  QString version (deliberate::ProgramVersion::Version());
  QStringList messages;
  messages.append (version);
  messages.append (configMessages);

  QMessageBox  box;
  box.setText (version);
  box.setDetailedText (messages.join ("\n"));
  QTimer::singleShot (30000, &box, SLOT (accept()));
  box.exec ();
}

void
AsRoute::Exiting ()
{
  QSize currentSize = size();
  Settings().setValue ("sizes/main",currentSize);
  Settings().sync();
}

void
AsRoute::License ()
{
  if (helpView) {
    helpView->Show ("qrc:/help/LICENSE.txt");
  }
}

void
AsRoute::FeatureButton ()
{
  QMessageBox::information (this, QString ("Info"), 
                     QString ("Feature Button clicked"));
}

void
AsRoute::LatLonButton ()
{
  QMessageBox::information (this, QString ("Info"), 
                      QString("LatLon Button clicked"));
}

void
AsRoute::ParcelButton ()
{
  QMessageBox::information (this, QString ("Info"), 
                      QString ("Parcel Button clicked"));
}

} // namespace
