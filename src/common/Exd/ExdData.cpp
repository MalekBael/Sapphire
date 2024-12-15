#include "ExdData.h"
#include <Logging/Logger.h>
#include <stdexcept>

using namespace Sapphire;

bool Data::ExdData::init( const std::string& path )
{
  try
  {
    Logger::info( "Initializing ExdData with path: {}", path );

    m_data = std::make_shared< xiv::dat::GameData >( path );
    if( !m_data )
    {
      Logger::error( "Failed to initialize GameData with path: {}", path );
      return false;
    }

    m_exd_data = std::make_shared< xiv::exd::ExdData >( *m_data );
    if( !m_exd_data )
    {
      Logger::error( "Failed to initialize ExdData with GameData." );
      return false;
    }

    Logger::info( "ExdData initialized successfully." );
    return true;
  } catch( const std::exception& e )
  {
    Logger::error( "Exception in ExdData::init: {}", e.what() );
    return false;
  } catch( ... )
  {
    Logger::error( "Unknown exception in ExdData::init." );
    return false;
  }
}
