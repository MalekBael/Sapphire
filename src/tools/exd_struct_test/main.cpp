
//#include <Exd/ExdDataGenerated.h>
#include <Exd/ExdData.h>
#include <Logging/Logger.h>

#include <datReader/Exd/Structs.h>

#include <Util/CrashHandler.h>

[[maybe_unused]] Sapphire::Common::Util::CrashHandler crashHandler;

xiv::dat::GameData* gameData = nullptr;

using namespace Sapphire;

namespace fs = std::filesystem;

//const std::string datLocation( "/opt/sapphire_3_15_0/bin/sqpack" );
const std::string datLocation( "C:\\Program Files (x86)\\SquareEnix\\FINAL FANTASY XIV - A Realm Reborn\\game\\sqpack" );

int main( int argc, char* argv[] )
{

  Logger::init( "struct_test" );

  Logger::info( "Setting up EXD data" );

  auto exdData = Data::ExdData();
  exdData.init( datLocation );

  auto row = exdData.getRow< Excel::ClassJob >( 1 );

  return 0;
}
