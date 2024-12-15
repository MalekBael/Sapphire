// NativeScriptMgr.cpp

#include "NativeScriptMgr.h"
#include "NativeScriptApi.h"

#include "WorldServer.h"
#include <Crypt/md5.h>
#include <Service.h>

namespace Sapphire::Scripting
{

  /*!
     * @brief Constructor for NativeScriptMgr.
     * Initializes the script loader's cache path using the WorldServer configuration.
     */
  NativeScriptMgr::NativeScriptMgr()
  {
    auto& server = Common::Service< Sapphire::World::WorldServer >::ref();
    m_loader.setCachePath( server.getConfig().scripts.cachePath );
  }

  /*!
     * @brief Loads a script from the specified path.
     *
     * This function loads a script module, retrieves its scripts, and registers them internally.
     *
     * @param path The filesystem path to the script module.
     * @return `true` if the script was loaded successfully; otherwise, `false`.
     */
  bool NativeScriptMgr::loadScript( const std::string& path )
  {
    std::scoped_lock lock( m_mutex );

    auto module = m_loader.loadModule( path );
    if( !module )
      return false;

    auto scripts = m_loader.getScripts( module->handle );
    if( !scripts )
    {
      m_loader.unloadScript( module );
      return false;
    }

    bool success = false;

    for( int i = 0;; i++ )
    {
      if( scripts[ i ] == nullptr )
        break;

      auto script = scripts[ i ];
      module->scripts.push_back( script );

      // Register the script in m_scripts
      m_scripts[ script->getType() ][ script->getId() ] = script;

      // If the script is of type BNpcScript, also add it to m_bnpcScripts
      BNpcScript* bnpcScript = dynamic_cast< BNpcScript* >( script );
      if( bnpcScript )
      {
        m_bnpcScripts[ bnpcScript->getId() ] = std::shared_ptr< BNpcScript >( bnpcScript );
      }

      success = true;
    }

    if( !success )
    {
      m_loader.unloadScript( module->handle );
      return false;
    }

    return true;
  }

  /*!
     * @brief Retrieves the module file extension based on the current platform.
     *
     * @return A string representing the module file extension (e.g., `.dll`, `.so`, `.dylib`).
     */
  const std::string NativeScriptMgr::getModuleExtension()
  {
    std::scoped_lock lock( m_mutex );
    return m_loader.getModuleExtension();
  }

  /*!
     * @brief Unloads a script module by its name.
     *
     * @param name The name of the script module to unload.
     * @return `true` if the module was unloaded successfully; otherwise, `false`.
     */
  bool NativeScriptMgr::unloadScript( const std::string& name )
  {
    std::scoped_lock lock( m_mutex );

    auto info = m_loader.getScriptInfo( name );
    if( !info )
      return false;

    return unloadScript( info );
  }

  /*!
     * @brief Unloads a script using its ScriptInfo pointer.
     *
     * This function removes all associated scripts from internal mappings and unloads the module.
     *
     * @param info A pointer to the ScriptInfo object representing the script module.
     * @return `true` if the script was unloaded successfully; otherwise, `false`.
     */
  bool NativeScriptMgr::unloadScript( ScriptInfo* info )
  {
    std::scoped_lock lock( m_mutex );

    for( auto& script : info->scripts )
    {
      // Remove the script from m_scripts
      auto type = script->getType();
      auto id = script->getId();
      if( m_scripts.find( type ) != m_scripts.end() )
      {
        m_scripts[ type ].erase( id );
        if( m_scripts[ type ].empty() )
        {
          m_scripts.erase( type );
        }
      }

      // If the script is a BNpcScript, remove it from m_bnpcScripts
      BNpcScript* bnpcScript = dynamic_cast< BNpcScript* >( script );
      if( bnpcScript )
      {
        m_bnpcScripts.erase( bnpcScript->getId() );
      }

      // Delete the script object to free memory
      delete script;
    }

    return m_loader.unloadScript( info );
  }

  /*!
     * @brief Queues a script module for reloading.
     *
     * @param name The name of the script module to reload.
     */
  void NativeScriptMgr::queueScriptReload( const std::string& name )
  {
    std::scoped_lock lock( m_mutex );

    auto info = m_loader.getScriptInfo( name );
    if( !info )
      return;

    // Backup the actual library path
    std::string libPath( info->library_path );

    if( !unloadScript( info ) )
      return;

    m_scriptLoadQueue.push( libPath );
  }

  /*!
     * @brief Processes the queue of scripts to be loaded.
     *
     * This function attempts to load each script in the queue. If loading fails, the script is deferred to the next processing cycle.
     */
  void NativeScriptMgr::processLoadQueue()
  {
    std::scoped_lock lock( m_mutex );

    std::vector< std::string > deferredLoads;

    while( !m_scriptLoadQueue.empty() )
    {
      auto item = m_scriptLoadQueue.front();

      // Attempt to load the script; defer if it fails
      if( !loadScript( item ) )
        deferredLoads.push_back( item );

      m_scriptLoadQueue.pop();
    }

    // Re-queue any deferred scripts for the next cycle
    for( auto& item : deferredLoads )
      m_scriptLoadQueue.push( item );
  }

  /*!
     * @brief Finds scripts matching the given search term.
     *
     * Performs a case-insensitive search for script modules and populates the provided set with matching ScriptInfo pointers.
     *
     * @param scripts A set to be populated with matching ScriptInfo pointers.
     * @param search The search term to match against script module names.
     */
  void NativeScriptMgr::findScripts( std::set< ScriptInfo* >& scripts, const std::string& search )
  {
    std::scoped_lock lock( m_mutex );
    m_loader.findScripts( scripts, search );
  }

  /*!
     * @brief Checks if a script module with the specified name is loaded.
     *
     * @param name The name of the script module to check.
     * @return `true` if the module is loaded; otherwise, `false`.
     */
  bool NativeScriptMgr::isModuleLoaded( const std::string& name )
  {
    std::scoped_lock lock( m_mutex );
    return m_loader.isModuleLoaded( name );
  }

  /*!
     * @brief Creates a new instance of NativeScriptMgr.
     *
     * @return A `std::shared_ptr` to the newly created `NativeScriptMgr` instance.
     */
  std::shared_ptr< NativeScriptMgr > createNativeScriptMgr()
  {
    return std::make_shared< NativeScriptMgr >();
  }

}// namespace Sapphire::Scripting

// Explicit template instantiation for commonly used script types (if needed)
// Uncomment and modify the following line based on your usage
// template BNpcScript* NativeScriptMgr::getScript<BNpcScript>(uint32_t scriptId);
