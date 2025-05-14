#include <cstdint>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <locale>
#include <set>
#include <string>
#include <unordered_map>

#include <DatCat.h>
#include <Exd.h>
#include <ExdCat.h>
#include <ExdData.h>
#include <File.h>
#include <GameData.h>

#include <Exd/ExdData.h>
#include <Logging/Logger.h>

#include <algorithm>

Sapphire::Data::ExdData g_exdDataGen;
namespace fs = std::filesystem;
using namespace Sapphire;

std::string javaPath( "java " );
std::string gamePath( "C:\\Program Files (x86)\\SquareEnix\\FINAL FANTASY XIV - A Realm Reborn\\game\\sqpack" );

const std::string onWithinRangeStr(
        "  void onWithinRange( World::Levequest& levequest, Entity::Player& player, uint64_t eRangeId, float x, float y, float z ) override\n"
        "  {\n"
        "  }\n\n" );

// Define the splito struct for the split function
struct splito {
  enum empties_t
  {
    empties_ok,
    no_empties
  };
};

// Reuse utility functions from quest_parser
bool compareNat( const std::string& a, const std::string& b )
{
  if( a.empty() )
    return true;
  if( b.empty() )
    return false;
  if( std::isdigit( a[ 0 ] ) && !std::isdigit( b[ 0 ] ) )
    return true;
  if( !std::isdigit( a[ 0 ] ) && std::isdigit( b[ 0 ] ) )
    return false;
  if( !std::isdigit( a[ 0 ] ) && !std::isdigit( b[ 0 ] ) )
  {
    if( std::toupper( a[ 0 ] ) == std::toupper( b[ 0 ] ) )
      return compareNat( a.substr( 1 ), b.substr( 1 ) );
    return ( std::toupper( a[ 0 ] ) < std::toupper( b[ 0 ] ) );
  }

  std::istringstream issa( a );
  std::istringstream issb( b );
  int ia, ib;
  issa >> ia;
  issb >> ib;
  if( ia != ib )
    return ia < ib;

  std::string anew, bnew;
  std::getline( issa, anew );
  std::getline( issb, bnew );
  return ( compareNat( anew, bnew ) );
}

std::string titleCase( const std::string& str )
{
  if( str.empty() )
    return str;

  std::string retStr( str );
  std::transform( str.begin(), str.end(), retStr.begin(), ::tolower );
  std::locale loc;
  retStr[ 0 ] = std::toupper( str[ 0 ], loc );
  for( size_t i = 1; i < str.size(); ++i )
  {
    if( str[ i - 1 ] == ' ' || str[ i - 1 ] == '_' ||
        ( std::isdigit( str[ i - 1 ], loc ) && !std::isdigit( str[ i ], loc ) ) )
      retStr[ i ] = std::toupper( str[ i ], loc );
  }
  return retStr;
}

std::string titleCaseNoUnderscores( const std::string& str )
{
  std::string result = titleCase( str );

  result.erase( std::remove_if( result.begin(), result.end(), []( const char c ) {
                  return c == '_';
                } ),
                result.end() );

  return result;
}

template< typename Container >
Container& split(
        Container& result,
        const typename Container::value_type& s,
        const typename Container::value_type& delimiters,
        splito::empties_t empties = splito::empties_ok )
{
  result.clear();
  size_t current;
  size_t next = -1;
  do
  {
    if( empties == splito::no_empties )
    {
      next = s.find_first_not_of( delimiters, next + 1 );
      if( next == Container::value_type::npos ) break;
      next -= 1;
    }
    current = next + 1;
    next = s.find_first_of( delimiters, current );
    result.push_back( s.substr( current, next - current ) );
  } while( next != Container::value_type::npos );
  return result;
}

// Helper functions similar to quest_parser but for levequest data
const std::string& getItemNameFromExd( uint32_t id )
{
  static std::unordered_map< uint32_t, std::string > itemNames;
  static std::string invalid;

  if( itemNames.empty() )
  {
    auto nameIdList = g_exdDataGen.getIdList< Excel::Item >();
    for( auto id : nameIdList )
    {
      auto itemName = g_exdDataGen.getRow< Excel::Item >( id );
      if( itemName && !itemName->getString( itemName->data().Text.SGL ).empty() )
        itemNames[ id ] = itemName->getString( itemName->data().Text.SGL );
    }
  }
  if( auto name = itemNames.find( id ); name != itemNames.end() )
    return name->second;

  return invalid;
}

const std::string& getActorPosFromLevelExd( uint32_t id )
{
  static std::unordered_map< uint32_t, std::string > levelPositions;
  static std::string invalid;

  if( levelPositions.empty() )
  {
    auto levelIdList = g_exdDataGen.getIdList< Excel::Level >();
    for( auto id : levelIdList )
    {
      auto levelRow = g_exdDataGen.getRow< Excel::Level >( id );
      if( levelRow )
      {
        auto assetType = levelRow->data().eAssetType;
        // enpc, bnpc, eobj
        if( assetType != 8 && assetType != 9 && assetType != 45 )
          continue;
        std::string pos( " ( Pos: " );
        pos +=
                std::to_string( levelRow->data().TransX ) + " " +
                std::to_string( levelRow->data().TransY ) + " " +
                std::to_string( levelRow->data().TransZ ) + "  " +
                "Teri: " + std::to_string( levelRow->data().TerritoryType ) + " )";
        levelPositions.emplace( levelRow->data().BaseId, pos );
      }
    }
  }
  if( auto pos = levelPositions.find( id ); pos != levelPositions.end() )
    return pos->second;
  return invalid;
}

const std::string& getActorNameFromExd( uint32_t id )
{
  static std::unordered_map< uint32_t, std::string > bnpcNames, enpcNames, eobjNames;
  static std::string invalid;

  // bnpc
  if( id < 1000000 )
  {
    if( bnpcNames.empty() )
    {
      auto nameIdList = g_exdDataGen.getIdList< Excel::BNpcName >();
      for( auto id : nameIdList )
      {
        auto BNpcName = g_exdDataGen.getRow< Excel::BNpcName >( id );
        if( BNpcName && !BNpcName->getString( BNpcName->data().Text.SGL ).empty() )
          bnpcNames[ id ] = BNpcName->getString( BNpcName->data().Text.SGL );
      }
    }
    if( auto name = bnpcNames.find( id ); name != bnpcNames.end() )
      return name->second;
  }
  // enpcresident
  else if( id < 2000000 )
  {
    if( enpcNames.empty() )
    {
      auto nameIdList = g_exdDataGen.getIdList< Excel::ENpcResident >();
      for( auto id : nameIdList )
      {
        auto eNpcName = g_exdDataGen.getRow< Excel::ENpcResident >( id );
        if( eNpcName && !eNpcName->getString( eNpcName->data().Text.SGL ).empty() )
          enpcNames[ id ] = eNpcName->getString( eNpcName->data().Text.SGL );
      }
    }
    if( auto name = enpcNames.find( id ); name != enpcNames.end() )
      return name->second;
  }
  // eobj
  else
  {
    if( eobjNames.empty() )
    {
      auto nameIdList = g_exdDataGen.getIdList< Excel::EObj >();
      for( auto id : nameIdList )
      {
        auto eObjName = g_exdDataGen.getRow< Excel::EObj >( id );
        if( eObjName && !eObjName->getString( eObjName->data().Text.SGL ).empty() )
          eobjNames[ id ] = eObjName->getString( eObjName->data().Text.SGL );
      }
    }
    if( auto name = eobjNames.find( id ); name != eobjNames.end() )
      return name->second;
  }
  return invalid;
}

// Function to check if a script file exists
bool scriptFileExists( const std::string& path, xiv::dat::GameData& data )
{
  try
  {
    data.getFile( path );
    return true;
  } catch( const std::exception& )
  {
    return false;
  }
}

// Function to get script name from leve ID
std::string getScriptNameFromLeveID(uint32_t leveId)
{
  // Generate script name based on leve ID
  return "leve_" + std::to_string(leveId);
}

void createLevequestScript( uint32_t leveId, std::set< std::string >& additionalList, std::vector< std::string >& functions )
{
  auto leveInfo = g_exdDataGen.getRow< Excel::Leve >( leveId );
  if( !leveInfo )
  {
    return;
  }

  std::string header(
          "// This is an automatically generated C++ script template for a levequest\n"
          "// Content needs to be added by hand to make it function\n"
          "// In order for this script to be loaded, move it to the correct folder in <root>/scripts/\n"
          "\n"
          "#include <Actor/Player.h>\n"
          "#include \"Manager/EventMgr.h\"\n"
          "#include <ScriptObject.h>\n"
          "#include <Service.h>\n\n" );

  std::vector< std::string > scenes;
  std::vector< std::string > sequences;
  std::vector< std::string > vars;
  bool hasEventItem = false;

  for( const auto& function : functions )
  {
    if( function.find( "GetEventItems" ) != std::string::npos )
    {
      hasEventItem = true;
    }
  }

  for( auto& entry : additionalList )
  {
    if( entry.find( "OnScene" ) != std::string::npos )
    {
      scenes.push_back( entry );
    }
    else if( entry.find( "SEQ" ) != std::string::npos )
    {
      if( entry.find( "SEQ_OFFER" ) == std::string::npos )
        sequences.push_back( entry );
    }
    else if( entry.find( "Flag" ) != std::string::npos ||
             entry.find( "LeveUI" ) != std::string::npos )
    {
      vars.push_back( entry.substr( std::string( "GetLeve" ).length() ) );
    }
  }

  // Generated class name from leve ID
  std::string className = "Leve" + std::to_string( leveId );

  // Extract levequest-specific information
  std::string leveTypeInfo = "    // Levequest Assignment Type: " + std::to_string( leveInfo->data().AssignmentType ) + "\n";
  std::string genreInfo = "    // Levequest Genre: " + std::to_string( leveInfo->data().Genre ) + "\n";
  std::string classjobInfo = "    // ClassJob: " + std::to_string( leveInfo->data().ClassJob ) + "\n";
  std::string levelInfo = "    // Level: " + std::to_string( leveInfo->data().ClassLevel ) + "\n";

  // Scene handler functions
  std::string sceneStr( "  //////////////////////////////////////////////////////////////////////\n"
                        "  // Available Scenes in this levequest, not necessarily all are used\n" );

  for( auto& scene : scenes )
  {
    if( scene.find( "OnScene" ) != std::string::npos )
    {
      std::string sceneName = scene.substr( 2 );
      std::string sceneId = scene.substr( 7 );

      std::size_t numOff = sceneId.find_first_not_of( "0" );
      sceneId = numOff != std::string::npos ? sceneId.substr( numOff ) : "0";

      std::string sceneFlags = "NONE";
      for( const auto& function : functions )
      {
        if( function.find( sceneName ) != std::string::npos )
        {
          if( function.find( "CutScene" ) != std::string::npos || function.find( "Cutscene" ) != std::string::npos )
          {
            sceneFlags = "FADE_OUT | CONDITION_CUTSCENE | HIDE_UI";
          }
          else if( function.find( "FadeIn" ) != std::string::npos )
          {
            sceneFlags = "FADE_OUT | HIDE_UI";
          }
          break;
        }
      }

      sceneStr += std::string(
              "  //////////////////////////////////////////////////////////////////////\n\n"
              "  void " +
              sceneName + "(World::Levequest& leve, Entity::Player& player)\n"
                          "  {\n"
                          "    eventMgr().playLeveScene(player, getId(), " +
              sceneId + ", " + sceneFlags + ", bindSceneReturn(&" + className + "::" + sceneName +
              "Return));\n"
              "  }\n\n"
              "  void " +
              sceneName + "Return(World::Levequest& leve, Entity::Player& player, const Event::SceneResult& result)\n"
                          "  {\n"
                          "    // Handle scene return actions\n"
                          "  }\n\n" );
    }
  }

  // Sequences and state variables
  std::string seqStr;
  seqStr.reserve( 0xFFF );
  seqStr += levelInfo;
  seqStr += classjobInfo;
  seqStr += genreInfo;
  seqStr += leveTypeInfo;
  seqStr += ( "    // Steps in this levequest (0 is before accepting, \n"
              "    // 1 is first, 255 means ready for turning it in\n" );
  std::string leveVarStr( "    // Levequest vars / flags used\n" );

  seqStr += "    enum Sequence : uint8_t\n    {\n";
  for( auto& seq : sequences )
  {
    if( seq.find( "SEQ_FINISH" ) != std::string::npos )
    {
      seqStr += "      SeqFinish = 255,\n";
    }
    else
    {
      std::string seqName = titleCaseNoUnderscores( seq );
      std::string seqId = seq.substr( 4 );
      seqStr += "      " + seqName + " = " + seqId + ",\n";
    }
  }
  seqStr += "    };\n";

  for( auto& var : vars )
  {
    leveVarStr += "    // " + var + "\n";
  }

  // Build entity listings from script data
  std::vector< std::string > script_entities;
  std::string sentities = "    // Entities found in the script data of the levequest\n";
  std::vector< std::string > actorList;

  // Extract specific entities from the script if we could find them
  for( const auto& entry : additionalList )
  {
    if( entry.find( "ACTOR" ) != std::string::npos ||
        entry.find( "NPC" ) != std::string::npos ||
        entry.find( "ENEMY" ) != std::string::npos )
    {
      actorList.push_back( titleCaseNoUnderscores( entry ) );
      script_entities.push_back( entry + " = 0; // Actor entity" );
    }
    else if( entry.find( "RITEM" ) != std::string::npos )
    {
      script_entities.push_back( entry + " = 0; // Item entity" );
    }
  }

  std::sort( script_entities.begin(), script_entities.end(), compareNat );
  for( auto& entity : script_entities )
  {
    auto name = titleCaseNoUnderscores( entity );
    sentities += "    static constexpr auto " + name + "\n";
  }

  // Additional info and script header
  std::string additional = "// Levequest ID: " + std::to_string( leveId ) + "\n";
  additional += "// Levequest Name: " + leveInfo->getString( leveInfo->data().Text.Name ) + "\n";

  // Find client (NPC who issues the leve)
  int32_t clientId = leveInfo->data().Client;
  additional += "// Client (Leve Issuer): " + std::to_string( clientId ) + " (" + getActorNameFromExd( clientId ) + ")\n";

  additional += "// Assignment Type: " + std::to_string( leveInfo->data().AssignmentType ) + "\n";
  additional += "// Town: " + std::to_string( leveInfo->data().Town ) + "\n";
  additional += "// Class Job: " + std::to_string( leveInfo->data().ClassJob ) + "\n\n";

  additional += "using namespace Sapphire;\n\n";

  // Event handlers
  std::string scriptEntry;
  scriptEntry.reserve( 0xFFFF );
  scriptEntry += "  //////////////////////////////////////////////////////////////////////\n  // Event Handlers\n";

  scriptEntry +=
          "  void onTalk(World::Levequest& leve, Entity::Player& player, uint64_t actorId) override\n"
          "  {\n";
  if( !actorList.empty() )
  {
    scriptEntry += "    switch(actorId)\n";
    scriptEntry += "    {\n";
    for( auto& actor : actorList )
    {
      scriptEntry += "      case " + actor + ":\n";
      scriptEntry += "      {\n";
      scriptEntry += "        break;\n";
      scriptEntry += "      }\n";
    }
    scriptEntry += "    }\n";
  }
  scriptEntry += "  }\n\n";

  // Add levequest-specific handlers
  scriptEntry +=
          "  void onLeveComplete(World::Levequest& leve, Entity::Player& player) override\n"
          "  {\n"
          "    // Handle levequest completion\n"
          "  }\n\n";

  scriptEntry +=
          "  void onLeveAccept(World::Levequest& leve, Entity::Player& player) override\n"
          "  {\n"
          "    // Handle levequest acceptance\n"
          "  }\n\n";

  // Special handlers for gathering/crafting leves if applicable
  if( leveInfo->data().AssignmentType == 1 || leveInfo->data().AssignmentType == 2 )
  {
    scriptEntry +=
            "  void onItemTurnin(World::Levequest& leve, Entity::Player& player, uint32_t itemId, uint32_t quantity) override\n"
            "  {\n"
            "    // Handle item turn-in for crafting/gathering leves\n"
            "  }\n\n";
  }

  // Special handlers for battle leves
  if( leveInfo->data().AssignmentType == 0 )
  {
    scriptEntry +=
            "  void onBNpcKill(World::Levequest& leve, Entity::BNpc& bnpc, Entity::Player& player) override\n"
            "  {\n"
            "    // Handle monster kills for battle leves\n"
            "  }\n\n";
  }

  // Constructor and class definition
  std::string constructor;
  constructor += std::string(
          "  private:\n"
          "    // Basic levequest information \n" );
  constructor += leveVarStr + "\n";
  constructor += seqStr + "\n";
  constructor += sentities + "\n";
  constructor += "  public:\n";
  constructor += "    " + className + "() : Sapphire::ScriptAPI::LevequestScript" + "(" + std::to_string( leveId ) + "){}; \n";
  constructor += "    ~" + className + "() = default; \n";

  std::string classString(
          "class " + className + " : public Sapphire::ScriptAPI::LevequestScript\n"
                                 "{\n" +
          constructor +
          "\n" +
          scriptEntry +
          "\n" +
          "  private:\n" +
          sceneStr +
          "};\n\nEXPOSE_SCRIPT(" + className + ");" );

  // Write the generated script to a file
  std::ofstream outputFile;
  outputFile.open( "generated/" + className + ".cpp" );
  outputFile << header << additional << classString;
  outputFile.close();
}

void parseIf( std::ifstream& t, std::string& functionBlock, std::string& line )
{
  if( line.find( " if " ) != std::string::npos )
  {
    while( true )
    {
      std::getline( t, line );
      functionBlock.append( line );
      if( line.find( " end" ) != std::string::npos )
        break;

      parseIf( t, functionBlock, line );
    }
  }
}

int main( int argc, char** argv )
{
  Logger::init( "levequest_parser" );

  bool unluac = false;

  if( argc > 1 )
    gamePath = std::string( argv[ 1 ] );
  if( argc > 2 )
    unluac = ( bool ) atoi( argv[ 2 ] );
  if( argc > 3 )
    javaPath = std::string( argv[ 3 ] );

  unluac = true;

  Logger::info( "Setting up generated EXD data" );
  if( !g_exdDataGen.init( gamePath ) )
  {
    std::cout << gamePath << "\n";
    Logger::fatal( "Error setting up EXD data " );
    std::cout << "Usage: levequest_parser \"path/to/ffxiv/game/sqpack\" <1/0 unluac export toggle>\n";
    return 0;
  }

  xiv::dat::GameData data( gamePath );

  // Use Leve table
  auto rows = g_exdDataGen.getIdList< Excel::Leve >();

  if( !fs::exists( "./generated" ) )
    fs::create_directory( "./generated" );

  Logger::info( "Export in progress" );

  auto updateInterval = rows.size() / 20;
  uint32_t i = 0;
  for( const auto& leveId : rows )
  {
    Logger::info( "Processing leve ID: {0}", leveId );

    // Try different potential script locations
    std::vector< std::string > scriptPaths;

    // Format: leve_[ID].luab
    scriptPaths.push_back( "game_script/leve/leve_" + std::to_string( leveId ) + ".luab" );

    // Format: lev_[ID].luab
    scriptPaths.push_back( "game_script/leve/lev_" + std::to_string( leveId ) + ".luab" );

    // Format based on job category
    auto leveInfo = g_exdDataGen.getRow< Excel::Leve >( leveId );
    if( leveInfo )
    {
      int jobType = leveInfo->data().ClassJob;
      // By assignment type
      std::string assignmentPrefix = "";
      switch( leveInfo->data().AssignmentType )
      {
        case 0:
          assignmentPrefix = "battle_";
          break;
        case 1:
          assignmentPrefix = "craft_";
          break;
        case 2:
          assignmentPrefix = "gather_";
          break;
        default:
          break;
      }

      if( !assignmentPrefix.empty() )
      {
        scriptPaths.push_back( "game_script/leve/" + assignmentPrefix + std::to_string( leveId ) + ".luab" );
      }
    }

    // Try all potential paths
    bool scriptFound = false;
    std::string validScriptPath;

    for( const auto& scriptPath : scriptPaths )
    {
      if( scriptFileExists( scriptPath, data ) )
      {
        validScriptPath = scriptPath;
        scriptFound = true;
        break;
      }
    }

    if( !scriptFound )
    {
      Logger::warn( "No script file found for leve ID: {0}, skipping", leveId );
      continue;
    }

    try
    {
      const xiv::dat::Cat& scriptCat = data.getCategory( "game_script" );
      const auto& scriptFile = data.getFile( validScriptPath );
      auto& section = scriptFile->access_data_sections().at( 0 );

      std::ofstream outputFile1;
      outputFile1.open( "generated/leve_" + std::to_string( leveId ) + ".luab", std::ios::binary );
      outputFile1.write( &section[ 0 ], section.size() );
      outputFile1.close();

      std::vector< std::string > functions;
      if( unluac )
      {
        std::string command =
                javaPath + " -jar unluac_2015_06_13.jar " + "generated/leve_" + std::to_string( leveId ) +
                ".luab" + ">> " + "generated/leve_" + std::to_string( leveId ) + ".lua";

        if( system( command.c_str() ) == -1 )
        {
          Logger::error( "Error executing java command:\n {0}\nerrno: {1}", command, std::strerror( errno ) );
          continue;
        }

        std::ifstream t;
        t.open( "generated/leve_" + std::to_string( leveId ) + ".lua" );
        std::string line;
        std::getline( t, line );// Skip first line
        while( t )
        {
          std::getline( t, line );
          std::string functionBlock;
          if( line.find( "function" ) != std::string::npos )
          {
            functionBlock.append( line );
            while( true )
            {
              std::getline( t, line );
              functionBlock.append( line );
              if( line.find( "end" ) != std::string::npos )
              {
                functions.push_back( functionBlock );
                break;
              }
              parseIf( t, functionBlock, line );
            }
          }
        }
        t.close();
      }

      // Extract string entries from the script file
      uint32_t offset = 0;
      std::set< std::string > stringList;
      while( offset < section.size() )
      {
        std::string entry( &section[ offset ] );
        offset += static_cast< uint32_t >( entry.size() + 1 );

        if( entry.size() > 3 && entry.find_first_not_of( "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_-" ) == std::string::npos )
        {
          if( entry.find( "SEQ" ) != std::string::npos ||
              entry.find( "LeveUI" ) != std::string::npos ||
              entry.find( "OnScene" ) != std::string::npos ||
              entry.find( "Flag" ) != std::string::npos ||
              entry.find( "ACTOR" ) != std::string::npos ||
              entry.find( "ENEMY" ) != std::string::npos ||
              entry.find( "NPC" ) != std::string::npos ||
              entry.find( "RITEM" ) != std::string::npos )
          {
            if( entry.find( "HOWTO" ) == std::string::npos )
              stringList.insert( entry );
          }
        }

        if( offset >= section.size() )
          break;
      }

      createLevequestScript( leveId, stringList, functions );
    } catch( const std::exception& e )
    {
      Logger::error( "Error processing levequest {0}: {1}", leveId, e.what() );
      continue;
    }

    ++i;
    if( i % updateInterval == 0 )
      std::cout << ".";
  }

  std::cout << "\nDone!";
  return 0;
}
