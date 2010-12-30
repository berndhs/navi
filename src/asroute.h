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

  void SendSomeRequests ();
  void KickRequestQueue ();
  void LatLonButton ();
  void ParcelButton ();
  void FeatureButton ();
  void HandleRangeNodes (int reqId, const QStringList & nodes);
  void HandleLatLon (int reqId, double lat, double lon);
  void HandleTagList (int reqId, const TagList & tagList);

private:

  void QueueAskNodeDetails (QTreeWidgetItem * item, 
                            const QString & nodeId);
  void AskLatLon (QTreeWidgetItem * item, const QString & nodeId);
  void AskNodeTagList (QTreeWidgetItem * item, const QString & nodeId);

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

  enum RequestType {
       Req_None = 0,
       Req_LatLon,
       Req_NodeTagList,
       Req_Tag,
       Req_Bad
  };

  struct ResponseStruct {
    RequestType          type;
    QTreeWidgetItem  *destItem;
  };

  struct RequestStruct {
    RequestType       type;
    QString           id;
    QTreeWidgetItem  *destItem;
  };
   

  void Connect ();
  void CloseCleanup ();
  void SetDefaults ();
  void ListNodes ();

  QApplication    *app;
  Ui_AsRouteMain   mainUi;
  AsDbManager      db;
  QStringList      configMessages;
  ConfigEdit       configEdit;
  deliberate::HelpView        *helpView;
  RouteCellMenu  *cellMenu;
  QMap <CellType, QString > cellTypeName;

  QSet <QString>  nodeSet;
  
  QMap <int, ResponseStruct>  requestInDB;
  QList <RequestStruct>       requestToSend;


} ;

} // namespace

#endif
