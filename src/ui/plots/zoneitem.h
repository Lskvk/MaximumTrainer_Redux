#ifndef ZONEITEM_H
#define ZONEITEM_H

#include "qwt_plot_zoneitem.h"

class ZoneItem : public QwtPlotZoneItem
{
public:
    ZoneItem(const QString &title );
    void setColor( const QColor &color );

};

#endif // ZONEITEM_H

