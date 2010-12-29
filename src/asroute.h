#ifndef ASROUTE_H
#define ASROUTE_H

#include "ui_asroute.h"
#include "config-edit.h"
#include "helpview.h"
#include "as-db-manager.h"
#include "navi-types.h"
#include "route-cell-menus.h"
#include <QMainWindow>
#include <QStringList>

class QApplication;

namespace navi
{
class AsRoute : public QMainWindow
{
Q_OBJECT

public:

  AsRoute (QWidget *parent=0);

  void Init (QApplication & qapp);
  void AddConfigMessages (const QStringList & argList);
  void Run ();
  void closeEvent ( QCloseEvent *event);

private slots:

  void Quit ();
  void EditSettings ();
  void SetSettings ();
  void About ();
  void License ();
  void Exiting ();

  void LatLonButton ();
  void ParcelButton ();
  void FeatureButton ();

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

  QApplication    *app;
  Ui_AsRouteMain   mainUi;
  AsDbManager      db;
  QStringList      configMessages;
  ConfigEdit       configEdit;
  deliberate::HelpView        *helpView;
  RouteCellMenu  *cellMenu;
  QMap <CellType, QString > cellTypeName;


} ;

} // namespace

#endif
