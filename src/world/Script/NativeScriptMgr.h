// NativeScriptMgr.h

#ifndef NATIVE_SCRIPT_MGR_H
#define NATIVE_SCRIPT_MGR_H

#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <unordered_map>

#include "Forwards.h"
#include "ScriptLoader.h"

// Include the BNpcScript header from the correct namespace
#include "../src/world/Script/NativeScriptApi.h"// Ensure correct path to NativeScriptApi.h

namespace Sapphire::Scripting
{
  // Bring BNpcScript and ScriptObject into the current namespace for easier reference
  using Sapphire::ScriptAPI::BNpcScript;
  using Sapphire::ScriptAPI::ScriptObject;

  /*!
     * @brief Contains all the functionality for easily loading, unloading, reloading and generally accessing scripts.
     */
  class NativeScriptMgr
  {
  private:
    // Correctly qualify BNpcScript with its namespace
    std::unordered_map< uint32_t, std::shared_ptr< BNpcScript > > m_bnpcScripts;
    std::mutex m_mutex;

  protected:
    /*!
         * @brief An internal list that maps script types to another list containing scripts indexed by their associated id
         */
    std::unordered_map< std::size_t, std::unordered_map< uint32_t, ScriptObject* > > m_scripts;

    ScriptLoader m_loader;

    /*!
         * @brief The queue that scripts to be reloaded are placed into.
         */
    std::queue< std::string > m_scriptLoadQueue;

    /*!
         * @brief Used to unload a script
         *
         * Clears m_scripts of any scripts associated with a ScriptInfo and then unloads that module
         *
         * @param info A pointer to the ScriptInfo object that is to be erased
         * @return true if successful, false if not
         */
    bool unloadScript( ScriptInfo* info );

  public:
    NativeScriptMgr();

    /*!
         * @brief Loads a script from a path
         *
         * This will populate m_scripts if it is successful and scripts will be available as soon as this returns.
         *
         * @param path The path to the module to load
         * @return true if successful, false if not
         */
    bool loadScript( const std::string& path );

    /*!
         * @brief Unloads a script
         *
         * @param name The module name of the script to unload
         * @return true if successful
         */
    bool unloadScript( const std::string& name );

    /*!
         * @brief Queues a script module to be reloaded
         *
         * Due to the nature of how this works, there's no return.
         * It will just silently fail over and over again to infinity and beyond until the server restarts... not that should ever happen under normal circumstances.
         *
         * @param name The name of the module to be reloaded.
         */
    void queueScriptReload( const std::string& name );

    /*!
         * @brief Case-insensitive search for modules, useful for debug commands
         *
         * @param scripts a set of ScriptInfo ptrs
         * @param search the search term
         */
    void findScripts( std::set< ScriptInfo* >& scripts, const std::string& search );

    /*!
         * @brief Called on a regular interval, allows for scripts to be loaded from the internal load queue.
         */
    void processLoadQueue();

    /*!
         * @brief Gets the module file extension for the current platform (windows, linux, osx)
         *
         * @return The file extension for the current platform
         */
    const std::string getModuleExtension();

    /*!
         * @brief Checks to see if a module with the specified name exists
         *
         * @param name The module name to lookup
         * @return true if loaded, false if not
         */
    bool isModuleLoaded( const std::string& name );

    /*!
         * @brief Get a specific script from the internal table
         *
         * @tparam T The type of the script to search for
         * @param scriptId The ID of the script to search for
         * @return T* if successful, nullptr if the script doesn't exist
         */
    template< typename T >
    T* getScript( uint32_t scriptId )
    {
      auto type = typeid( T ).hash_code();

      auto typeIt = m_scripts.find( type );
      if( typeIt == m_scripts.end() )
        return nullptr;

      auto scriptIt = typeIt->second.find( scriptId );
      if( scriptIt == typeIt->second.end() )
        return nullptr;

      return dynamic_cast< T* >( scriptIt->second );
    }
  };

  /*!
     * @brief Creates an instance of NativeScriptMgr
     *
     * @return a std::shared_ptr to NativeScriptMgr
     */
  std::shared_ptr< NativeScriptMgr > createNativeScriptMgr();
}// namespace Sapphire::Scripting

#endif// NATIVE_SCRIPT_MGR_H
