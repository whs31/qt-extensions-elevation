//
// Created by whs31 on 29.09.23.
//

#include "qtexelevation-tile.h"
#include <QtCore/QString>
#include <QtCore/QDataStream>
#include <QtExtensions/Math>
#include <QtExtensions/Logging>
#include <gdal_priv.h>

namespace QtEx
{
  Tile::Tile(const String& path, i8 latitude, i16 longitude)
    : m_tllat(0)
    , m_tllon(0)
    , m_latsize(0)
    , m_lonsize(0)
    , m_tilexsize(0)
    , m_tileysize(0)
    , m_tile({})
  {
    String f = path + "/";
    if(latitude >= 0)
    {
      if(longitude < 0) f += "0/";
      else f += "1/";
    }
    else
    {
      if(longitude < 0) f += "2/";
      else f += "3/";
    }
    f += String("%1/%2.tif").arg(abs<double>(latitude)).arg(abs<double>(longitude));
    auto dataset = (GDALDataset*)GDALOpen(f.toLocal8Bit().data(), GA_ReadOnly);
    if(dataset)
    {
      f64 adfGeoTransform[6];
      dataset->GetGeoTransform(adfGeoTransform);
      m_latsize = adfGeoTransform[5];
      m_lonsize = adfGeoTransform[1];
      m_tllat = adfGeoTransform[3] + m_latsize / 2.;
      m_tllon = adfGeoTransform[0] + m_lonsize / 2.;

      if(latitude + 1 == floor(m_tllat) and longitude == ceil(m_tllon))
      {
        GDALRasterBand* band = dataset->GetRasterBand(1);
        m_tilexsize = band->GetXSize();
        m_tileysize = band->GetYSize();
        m_tile = ByteArray(static_cast<int>(m_tilexsize * m_tileysize * sizeof(u16)), 0x00);
        if(band->RasterIO(GF_Read, 0, 0, m_tilexsize, m_tileysize, m_tile.data(), m_tilexsize, m_tileysize, GDT_Int16, 0, 0))
          m_tile = ByteArray();
      }
    }
  }

  auto Tile::elevation(double latitude, double longitude) const -> expected<f32, ElevationError>
  {
    if(not m_tile.size())
      return unexpected(ElevationError::TileNotFound);

    auto index = static_cast<u32>((round((latitude - m_tllat) / m_latsize) * m_tilexsize
                                         + round((longitude - m_tllon) / m_lonsize)) * sizeof(i16));
    QDataStream stream(m_tile);
    stream.setByteOrder(QDataStream::LittleEndian);
    i16 elevationValue;
    stream.skipRawData(static_cast<int>(index));
    stream >> elevationValue;
    return static_cast<f32>(elevationValue);
  }
} // QtEx