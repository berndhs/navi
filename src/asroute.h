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
#include <map>

class QApplication;

using namespace std;

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
  void FeatureButton ();
  void HandleRangeNodes (int reqId, const NaviNodeList & nodes);
  void HandleLatLon (int reqId, double lat, double lon);
  void HandleTagList (int reqId, const TagList & tagList);
  void HandleWayList (int reqId, const QStringList & wayList);
  void HandleWayTurnList (int reqId, const WayTurnList & wayList);
  void HandleRangeNodeTags (int reqId, const TagRecordList & tagList);
  void ChangeMaxCount (int newmax);
  void FindWays ();
  void CatchMark (int markId);


private:

  void QueueAskNodeDetails (const QString & nodeId);
  void AskLatLon (const QString & nodeId);
  void AskNodeTagList (const QString & nodeId);
  void UpdateLoad ();
  void Mark (const QString & message = QString ("Mark"));
  void QueueMark (const QString & message = QString ("Queued Mark"));
  void MakeRed (const QString & wayId);

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
       Req_Mark,
       Req_Bad
  };

  struct ResponseStruct {
    RequestType          type;
    QString           id;
  };

  struct RequestStruct {
    RequestType       type;
    QString           id;
  };
   
  struct MarkStruct {
    int      markId;
    int      elapsedStart;
    QString  markText;
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
  QMap <QString, TagItemType>  tagMap;
  
  QMap <int, ResponseStruct>  requestInDB;
  QList <RequestStruct>       requestToSend;

  int    numNodeDetails;
  int    numNodes;

  int    numQueries;
  QTime  markClock;
  QMap <int, MarkStruct>  markMap;

  multimap <QString, WayTurn>   turnMap;
  QStringList                  redWays;

  QMap <QString, QVector2D>   nodeCoords;
  QString   localPrefix;

} ;

} // namespace

#endif
