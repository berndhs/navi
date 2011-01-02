#ifndef ASROUTE_H
#define ASROUTE_H

#include "ui_asroute.h"
#include "map-display.h"
#include "config-edit.h"
#include "helpview.h"
#include "as-db-manager.h"
#include "navi-types.h"
#include "route-cell-menus.h"
#include <QMainWindow>
#include <QStringList>
#include <QVector2D>
#include <QPoint>
#include <QMap>

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

  void ShowMap ();
  void HideMap ();
  void DrawMap ();

  void Clear ();
  void SendSomeRequests ();
  void KickRequestQueue ();
  void LatLonButton ();
  void ParcelButton ();
  void FeatureButton ();
  void HandleRangeNodes (int reqId, const QStringList & nodes);
  void HandleLatLon (int reqId, double lat, double lon);
  void HandleTagList (int reqId, const TagList & tagList);
  void HandleWayList (int reqId, const QStringList & wayList);
  void ChangeMaxCount (int newmax);
  void FindWays ();


private:

  void QueueAskNodeDetails (QTreeWidgetItem * item, 
                            const QString & nodeId);
  void AskLatLon (QTreeWidgetItem * item, const QString & nodeId);
  void AskNodeTagList (QTreeWidgetItem * item, const QString & nodeId);
  void UpdateLoad ();
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

  enum CellDataType {
       Data_NodeId = Qt::UserRole+1,
       Data_WayId,
       Data_Lat,
       Data_Lon
  };

  enum RequestType {
       Req_None = 0,
       Req_LatLon,
       Req_NodeTagList,
       Req_WayList,
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

  MapDisplay      *mapWidget;
  AsDbManager      db;

  int              maxSend;
  int              maxPending;

  QStringList      configMessages;
  ConfigEdit       configEdit;
  deliberate::HelpView        *helpView;
  RouteCellMenu  *cellMenu;
  QMap <CellType, QString > cellTypeName;

  QSet <QString>  nodeSet;
  QSet <QString>  waySet;
  
  QMap <int, ResponseStruct>  requestInDB;
  QList <RequestStruct>       requestToSend;

  int    numNodeDetails;
  int    numNodes;

  QMap <QString, QVector2D>   nodeCoords;

} ;

} // namespace

#endif
