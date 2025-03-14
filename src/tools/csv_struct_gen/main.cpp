#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

struct ColumnDef {
  std::string name;
  std::string type;
  size_t arraySize = 0;
  bool isArray = false;
  bool isBitfield = false;
  int bitSize = 0;
};

struct StructDef {
  std::string id;
  std::string name;
  std::vector< ColumnDef > fields;
  size_t calculatedSize = 0;
  size_t headerSize = 0;
};

// Convert CSV column type to C++ type
std::string mapToCppType( const std::string& csvType )
{
  if( csvType == "str" || csvType == "string" )
    return "Excel::StringOffset";
  else if( csvType == "bool" )
    return "bool";
  else if( csvType == "sbyte" || csvType == "int8" )
    return "int8_t";
  else if( csvType == "byte" || csvType == "uint8" )
    return "uint8_t";
  else if( csvType == "sint16" || csvType == "int16" )
    return "int16_t";
  else if( csvType == "uint16" )
    return "uint16_t";
  else if( csvType == "int32" )
    return "int32_t";
  else if( csvType == "uint32" )
    return "uint32_t";
  else if( csvType == "float32" )
    return "float";
  else if( csvType == "uint64" )
    return "uint64_t";

  // Default fallback
  std::cerr << "Unknown type: " << csvType << ", defaulting to int32_t\n";
  return "int32_t";
}

// Parse CSV file and extract column definitions
bool parseCSV( const std::string& csvPath, StructDef& structDef )
{
  std::ifstream file( csvPath );
  if( !file.is_open() )
  {
    std::cerr << "Failed to open CSV file: " << csvPath << std::endl;
    return false;
  }

  std::string line;
  std::vector< std::string > headers;
  std::vector< std::string > types;

  // Read headers
  if( std::getline( file, line ) )
  {
    std::stringstream ss( line );
    std::string cell;

    while( std::getline( ss, cell, ',' ) )
    {
      if( !cell.empty() && cell[ 0 ] == '"' && cell[ cell.length() - 1 ] == '"' )
        cell = cell.substr( 1, cell.length() - 2 );
      headers.push_back( cell );
    }
  }

  // Read types
  if( std::getline( file, line ) )
  {
    std::stringstream ss( line );
    std::string cell;

    while( std::getline( ss, cell, ',' ) )
    {
      if( !cell.empty() && cell[ 0 ] == '"' && cell[ cell.length() - 1 ] == '"' )
        cell = cell.substr( 1, cell.length() - 2 );
      types.push_back( cell );
    }
  }

  if( headers.size() != types.size() )
  {
    std::cerr << "CSV headers and types count mismatch in: " << csvPath << std::endl;
    return false;
  }

  // Extract filename without extension
  fs::path p( csvPath );
  structDef.name = p.stem().string();

  // Process columns
  for( size_t i = 0; i < headers.size(); i++ )
  {
    ColumnDef col;
    col.name = headers[ i ];

    // Check for array notation like BaseParam[6]
    std::regex arrayPattern( "(.+)\\[(\\d+)\\]" );
    std::smatch matches;

    if( std::regex_match( col.name, matches, arrayPattern ) )
    {
      col.name = matches[ 1 ].str();
      col.isArray = true;
      col.arraySize = std::stoi( matches[ 2 ].str() );
    }

    // Handle bitfields (typically flags in game data)
    if( types[ i ].find( "bit" ) != std::string::npos )
    {
      col.isBitfield = true;
      // Extract bit size if specified like "bit&1"
      size_t bitPos = types[ i ].find( "&" );
      if( bitPos != std::string::npos )
      {
        col.bitSize = std::stoi( types[ i ].substr( bitPos + 1 ) );
      }
      else
      {
        col.bitSize = 1;// Default to 1 bit
      }
      col.type = "uint8_t";
    }
    else
    {
      col.type = mapToCppType( types[ i ] );
    }

    structDef.fields.push_back( col );
  }

  return true;
}

// Calculate struct size and add padding as needed
void calculateStructSize( StructDef& structDef )
{
  size_t currentSize = 0;
  size_t bitfieldAccumulator = 0;
  std::string lastBitfieldType;

  for( const auto& field : structDef.fields )
  {
    size_t fieldSize = 0;

    if( field.isBitfield )
    {
      if( lastBitfieldType != field.type || bitfieldAccumulator + field.bitSize > 8 )
      {
        // Align to next byte boundary when switching types or exceeding 8 bits
        currentSize += ( bitfieldAccumulator > 0 ) ? sizeof( uint8_t ) : 0;
        bitfieldAccumulator = field.bitSize;
        lastBitfieldType = field.type;
      }
      else
      {
        bitfieldAccumulator += field.bitSize;
      }
      continue;
    }

    // Flush any pending bitfields
    if( bitfieldAccumulator > 0 )
    {
      currentSize += sizeof( uint8_t );
      bitfieldAccumulator = 0;
    }

    if( field.type == "Excel::StringOffset" )
      fieldSize = sizeof( uint32_t );
    else if( field.type == "bool" )
      fieldSize = sizeof( bool );
    else if( field.type == "int8_t" || field.type == "uint8_t" )
      fieldSize = 1;
    else if( field.type == "int16_t" || field.type == "uint16_t" )
      fieldSize = 2;
    else if( field.type == "int32_t" || field.type == "uint32_t" || field.type == "float" )
      fieldSize = 4;
    else if( field.type == "uint64_t" )
      fieldSize = 8;

    if( field.isArray )
    {
      fieldSize *= field.arraySize;
    }

    // Alignment padding
    if( fieldSize > 1 )
    {
      size_t alignment = fieldSize;
      size_t padding = ( alignment - ( currentSize % alignment ) ) % alignment;
      currentSize += padding;
    }

    currentSize += fieldSize;
  }

  // Add final bitfield bytes if any
  if( bitfieldAccumulator > 0 )
  {
    currentSize += sizeof( uint8_t );
  }

  // Calculate padding to align to 8-byte boundary (common alignment requirement)
  size_t paddingSize = ( 8 - ( currentSize % 8 ) ) % 8;

  structDef.calculatedSize = currentSize + paddingSize;
}

// Generate struct definition code
std::string generateStructCode( const StructDef& structDef )
{
  std::stringstream ss;

  ss << "/* " << structDef.id << " */\n";
  ss << "struct " << structDef.name << "\n{\n";

  // Group bitfields together
  std::map< std::string, std::vector< ColumnDef > > bitfieldGroups;
  int currentBitPos = 0;
  std::string currentType;

  for( const auto& field : structDef.fields )
  {
    if( field.isBitfield )
    {
      if( currentType != field.type || currentBitPos + field.bitSize > 8 )
      {
        currentBitPos = 0;
        currentType = field.type;
      }

      auto& group = bitfieldGroups[ currentType + std::to_string( currentBitPos / 8 ) ];
      group.push_back( field );
      currentBitPos += field.bitSize;
    }
    else
    {
      // Regular field
      ss << "  " << field.type;
      if( field.isArray )
      {
        ss << " " << field.name << "[" << field.arraySize << "]";
      }
      else
      {
        ss << " " << field.name;
      }
      ss << ";\n";
    }
  }

  // Write out bitfield groups
  for( const auto& group : bitfieldGroups )
  {
    for( const auto& field : group.second )
    {
      ss << "  " << field.type << " " << field.name << " : " << field.bitSize << ";\n";
    }
  }

  // Add padding if needed to match expected size
  if( structDef.headerSize > structDef.calculatedSize )
  {
    size_t paddingBytes = structDef.headerSize - structDef.calculatedSize;
    ss << "  int8_t padding0[" << paddingBytes << "];\n";
  }

  ss << "};\n\n";

  return ss.str();
}

// Extract struct ID from a comment line
std::string extractStructId( const std::string& line )
{
  std::regex idPattern( "/\\* (\\d+) \\*/" );
  std::smatch matches;

  if( std::regex_search( line, matches, idPattern ) && matches.size() > 1 )
  {
    return matches[ 1 ].str();
  }

  return "";
}

// Process all CSV files and update Structs.h
int main( int argc, char* argv[] )
{
  if( argc < 3 )
  {
    std::cerr << "Usage: " << argv[ 0 ] << " <csv_directory> <structs_header_path>\n";
    return 1;
  }

  std::string csvDir = argv[ 1 ];
  std::string structsPath = argv[ 2 ];

  // Read existing Structs.h to preserve non-generated content
  std::ifstream structsFile( structsPath );
  if( !structsFile.is_open() )
  {
    std::cerr << "Failed to open Structs.h at: " << structsPath << std::endl;
    return 1;
  }

  std::string line;
  std::map< std::string, std::string > existingStructs;
  std::string currentId;
  std::stringstream currentStruct;

  bool inStruct = false;

  // Parse existing structs
  while( std::getline( structsFile, line ) )
  {
    if( !inStruct )
    {
      std::string id = extractStructId( line );
      if( !id.empty() )
      {
        currentId = id;
        inStruct = true;
        currentStruct.str( "" );
        currentStruct << line << "\n";
      }
    }
    else
    {
      currentStruct << line << "\n";
      if( line == "};" )
      {
        existingStructs[ currentId ] = currentStruct.str();
        inStruct = false;
      }
    }
  }
  structsFile.close();

  // Process CSV files
  std::vector< StructDef > newStructs;
  for( const auto& entry : fs::directory_iterator( csvDir ) )
  {
    if( entry.path().extension() == ".csv" )
    {
      StructDef structDef;
      if( parseCSV( entry.path().string(), structDef ) )
      {
        // Try to match with existing struct ID
        for( const auto& [ id, def ] : existingStructs )
        {
          if( def.find( structDef.name ) != std::string::npos )
          {
            structDef.id = id;
            break;
          }
        }

        // If no ID found, generate a new one
        if( structDef.id.empty() )
        {
          // Generate a unique ID (would need proper implementation)
          structDef.id = "99999";// Placeholder
        }

        calculateStructSize( structDef );
        newStructs.push_back( structDef );
      }
    }
  }

  // Generate output - USE OFSTREAM INSTEAD OF IFSTREAM FOR WRITING
  std::ofstream outputFile( structsPath );
  if( !outputFile.is_open() )
  {
    std::cerr << "Failed to open Structs.h for writing at: " << structsPath << std::endl;
    return 1;
  }

  // Read header template (everything before first struct)
  std::ifstream templateHeader( structsPath );
  std::string headerTemplate;
  bool foundFirstStruct = false;

  while( std::getline( templateHeader, line ) )
  {
    if( line.find( "struct" ) != std::string::npos && line.find( ";" ) == std::string::npos )
    {
      foundFirstStruct = true;
      break;
    }
    headerTemplate += line + "\n";
  }
  templateHeader.close();

  // Write header
  outputFile << headerTemplate;

  // Write structs
  for( const auto& structDef : newStructs )
  {
    outputFile << generateStructCode( structDef );
  }

  outputFile.close();

  std::cout << "Successfully processed " << newStructs.size() << " CSV files and updated Structs.h\n";
  return 0;
}
