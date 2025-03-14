#include <DatCat.h>
#include <Exd.h>
#include <Exd/ExdData.h>
#include <ExdCat.h>
#include <ExdData.h>
#include <File.h>
#include <GameData.h>
#include <Logging/Logger.h>
#include <Util/Util.h>
#include <algorithm>
#include <cctype>
#include <iostream>
#include <set>
#include <variant>

#include <fstream>

Sapphire::Data::ExdData g_exdData;

using namespace Sapphire;

//const std::string datLocation( "/opt/sapphire_3_15_0/bin/sqpack" );
std::string datLocation( "C:\\Program Files (x86)\\SquareEnix\\FINAL FANTASY XIV - A Realm Reborn\\game\\sqpack" );

std::string generateEnum( const std::string& exd, int8_t nameIndex, const std::string& type, bool useLang = true )
{
  xiv::dat::GameData data( datLocation );
  xiv::exd::ExdData eData( data );

  std::map< std::string, uint32_t > nameMap;

  std::string result = "\n   ///////////////////////////////////////////////////////////\n";
  result += "   //" + exd + ".exd\n";
  result += "   enum class " + exd + " : " + type + "\n";
  result += "   {\n";

  // Get the category directly from the exd data
  auto& cat = eData.get_category( exd );
  auto lang = useLang ? xiv::exd::Language::en : xiv::exd::Language::none;
  auto sheet = cat.get_data( lang );
  auto rows = sheet.get_rows();

  for( auto& row : rows )
  {
    uint32_t id = row.first;
    auto& fields = row.second;

    // Use field access with safety checks
    if( nameIndex >= fields.size() )
      continue;

    std::string value;
    try
    {
      // Get the field directly
      auto& field = fields.at( nameIndex );

      // Since field is now a std::variant, check if it contains a string
      if( std::holds_alternative< std::string >( field ) )
      {
        value = std::get< std::string >( field );
      }
      else
      {
        continue;
      }
    } catch( const std::exception& )
    {
      continue;
    }

    std::string remove = ",_-':!(){} \x02\x1f\x01\x03";
    Common::Util::eraseAllIn( value, remove );

    if( value.empty() )
      continue;

    value[ 0 ] = std::toupper( value[ 0 ] );

    auto it = nameMap.find( value );
    if( it != nameMap.end() )
    {
      nameMap[ value ]++;
      value = value + std::to_string( nameMap[ value ] );
    }
    else
    {
      nameMap[ value ] = 0;
    }

    result += "      " + value + " = " + std::to_string( id ) + ",\n";
  }

  result += "   };\n";

  return result;
}

int main( int argc, char** argv )
{
  Logger::init( "commongen" );

  Logger::info( "Setting up EXD data" );

  if( argc > 1 )
  {
    Logger::info( "using dat path: {0}", std::string( argv[ 1 ] ) );
    datLocation = std::string( argv[ 1 ] );
  }

  if( !g_exdData.init( datLocation ) )
  {
    Logger::fatal( "Error setting up EXD data " );
    return 1;
  }

  std::string result = "#ifndef _COMMON_GEN_H_\n#define _COMMON_GEN_H_\n";

  result += "\n#include <stdint.h>\n\n";

  result +=
          "/* This file has been automatically generated.\n   Changes will be lost upon regeneration.\n   To change the content edit tools/exd_common_gen */\n";


  result += "namespace Sapphire::Common {\n";
  result += generateEnum( "ActionCategory", 0, "uint8_t" );
  result += generateEnum( "BaseParam", 1, "uint8_t" );
  result += generateEnum( "BeastReputationRank", 1, "uint8_t" );
  result += generateEnum( "BeastTribe", 11, "uint8_t" );
  result += generateEnum( "ClassJob", 0, "uint8_t" );
  result += generateEnum( "ContentType", 0, "uint8_t" );
  result += generateEnum( "EmoteCategory", 0, "uint8_t" );
  result += generateEnum( "ExVersion", 0, "uint8_t" );
  result += generateEnum( "GrandCompany", 0, "uint8_t" );
  result += generateEnum( "GuardianDeity", 0, "uint8_t" );
  result += generateEnum( "ItemUICategory", 0, "uint8_t" );
  result += generateEnum( "ItemSearchCategory", 0, "uint8_t" );
  result += generateEnum( "OnlineStatus", 2, "uint8_t" );
  result += generateEnum( "Race", 1, "uint8_t" );
  result += generateEnum( "Tribe", 0, "uint8_t" );
  result += generateEnum( "Town", 0, "uint8_t" );
  result += generateEnum( "Weather", 1, "uint8_t" );
  result += generateEnum( "HousingAppeal", 0, "uint8_t" );
  result += "}\n#endif\n";

  Logger::info( result );

  // Write to a file
  std::ofstream outFile( "Common.h" );
  if( outFile.is_open() )
  {
    outFile << result;
    outFile.close();
    Logger::info( "Generated Common.h successfully" );
  }
  else
  {
    Logger::error( "Failed to write to Common.h" );
  }

  return 0;
}
