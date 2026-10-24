#include "curvedataspeed.h"
#include "dataspeed.h"


const DataSpeed &CurveDataSpeed::values() const
{
    return DataSpeed::instance();
}

DataSpeed &CurveDataSpeed::values()
{
    return DataSpeed::instance();
}

QPointF CurveDataSpeed::sample( size_t i ) const
{
    return DataSpeed::instance().value( static_cast<int>( i ) );
}

size_t CurveDataSpeed::size() const
{
    return DataSpeed::instance().size();
}

QRectF CurveDataSpeed::boundingRect() const
{
    return DataSpeed::instance().boundingRect();
}


