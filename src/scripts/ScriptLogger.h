#ifndef _SCRIPTLOGGER_H
#define _SCRIPTLOGGER_H

#include <chrono>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace Sapphire::ScriptAPI
{
  class ScriptLogger
  {
  public:
    static void debug( const std::string& message )
    {
      std::cout << formatTimestamp() << " [debug] " << message << std::endl;
    }

    static void info( const std::string& message )
    {
      std::cout << formatTimestamp() << " [info] " << message << std::endl;
    }

    static void error( const std::string& message )
    {
      std::cerr << formatTimestamp() << " [error] " << message << std::endl;
    }

    template< typename... Args >
    static void debug( const std::string& format, Args&&... args )
    {
      debug( formatString( format, std::forward< Args >( args )... ) );
    }

    template< typename... Args >
    static void info( const std::string& format, Args&&... args )
    {
      info( formatString( format, std::forward< Args >( args )... ) );
    }

    template< typename... Args >
    static void error( const std::string& format, Args&&... args )
    {
      error( formatString( format, std::forward< Args >( args )... ) );
    }

  private:
    // Format current time as [HH:MM:SS.mmm] to match existing logs
    static std::string formatTimestamp()
    {
      auto now = std::chrono::system_clock::now();
      auto nowMs = std::chrono::duration_cast< std::chrono::milliseconds >( now.time_since_epoch() ) % 1000;
      auto timer = std::chrono::system_clock::to_time_t( now );

      std::tm bt;
#ifdef _WIN32
      localtime_s( &bt, &timer );
#else
      localtime_r( &timer, &bt );
#endif

      std::ostringstream timestamp;
      timestamp << "["
                << std::setfill( '0' ) << std::setw( 2 ) << bt.tm_hour << ":"
                << std::setfill( '0' ) << std::setw( 2 ) << bt.tm_min << ":"
                << std::setfill( '0' ) << std::setw( 2 ) << bt.tm_sec << "."
                << std::setfill( '0' ) << std::setw( 3 ) << nowMs.count() << "]";

      return timestamp.str();
    }

    // Simple formatter that replaces {0}, {1}, etc. with the provided arguments
    template< typename... Args >
    static std::string formatString( const std::string& format, Args&&... args )
    {
      if constexpr( sizeof...( args ) == 0 )
        return format;

      // Convert all arguments to strings and store in vector
      std::vector< std::string > argStrings;
      ( argStrings.push_back( toString( args ) ), ... );

      std::string result = format;

      // Replace placeholders {0}, {1}, etc. with argument strings
      for( size_t i = 0; i < argStrings.size(); ++i )
      {
        std::string placeholder = "{" + std::to_string( i ) + "}";
        size_t pos = result.find( placeholder );

        while( pos != std::string::npos )
        {
          result.replace( pos, placeholder.length(), argStrings[ i ] );
          pos = result.find( placeholder, pos + argStrings[ i ].length() );
        }
      }

      return result;
    }

    // Convert any type to string using a stringstream
    template< typename T >
    static std::string toString( const T& value )
    {
      std::ostringstream ss;
      ss << value;
      return ss.str();
    }

    // Special handling for bool to display "true"/"false" instead of 1/0
    static std::string toString( bool value )
    {
      return value ? "true" : "false";
    }

    // Special handling for char* and string literals
    static std::string toString( const char* value )
    {
      return std::string( value );
    }

    // Special handling for std::string to avoid unnecessary conversion
    static std::string toString( const std::string& value )
    {
      return value;
    }
  };
}// namespace Sapphire::ScriptAPI

#endif