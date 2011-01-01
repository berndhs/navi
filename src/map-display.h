#ifndef NAVI_MAP_DISPLAY_H
#define NAVI_MAP_DISPLAY_H

#include "ui_map-display.h"

#include <QPointF>
#include <QList>

class QPainter;

namespace navi
{
class MapDisplay : public QWidget
{
Q_OBJECT

public:

  MapDisplay (QWidget * parent=0);

  void AddPoint (const QPointF & p);
  void ClearPoints ();

protected:

  void paintEvent (QPaintEvent * event);

private:

  void PaintPoints (QPainter * painter);
  QPointF Scale (const QPointF p);
  void    SetRange ();

  Ui_MapDisplay   ui;

  QList <QPointF>  points;
  double           xLo;
  double           yLo;
  double           xHi;
  double           yHi;

  double           xRange;
  double           yRange;
  double           xScale;
  double           yScale;

};

} // namespace


#endif