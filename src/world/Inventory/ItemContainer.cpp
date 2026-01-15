#include <Common.h>
#include <Logging/Logger.h>
#include <Database/DatabaseDef.h>
#include <Service.h>

#include "Actor/Player.h"

#include "Item.h"
#include "Forwards.h"
#include "ItemContainer.h"

Sapphire::ItemContainer::ItemContainer( uint16_t storageId, uint16_t maxSize, const std::string& tableName,
                                        bool isMultiStorage, bool isPersistentStorage ) : m_id( storageId ),
                                                                                          m_size( maxSize ),
                                                                                          m_tableName( tableName ),
                                                                                          m_bMultiStorage( isMultiStorage ),
                                                                                          m_isPersistentStorage( isPersistentStorage )
{
}

Sapphire::ItemContainer::~ItemContainer()
{
}

uint16_t Sapphire::ItemContainer::getId() const
{
  return m_id;
}

uint16_t Sapphire::ItemContainer::getEntryCount() const
{
  return static_cast< uint16_t >( m_itemMap.size() );
}

void Sapphire::ItemContainer::removeItem( uint16_t slotId, bool removeFromDb )
{
  auto it = m_itemMap.find( slotId );
  if( it != m_itemMap.end() )
  {
    if( m_isPersistentStorage && removeFromDb )
    {
      auto& db = Common::Service< Db::DbWorkerPool< Db::ZoneDbConnection > >::ref();
      auto stmt = db.getPreparedStatement( Db::CHARA_ITEMGLOBAL_DELETE );
      stmt->setUInt64( 1, it->second->getUId() );
      db.execute( stmt );
    }

    m_itemMap.erase( it );
    Logger::debug( "Dropped item from slot {0}", slotId );
  }
}


Sapphire::ItemMap& Sapphire::ItemContainer::getItemMap()
{
  return m_itemMap;
}

const Sapphire::ItemMap& Sapphire::ItemContainer::getItemMap() const
{
  return m_itemMap;
}

int16_t Sapphire::ItemContainer::getFreeSlot()
{
  for( uint16_t slotId = 0; slotId < m_size; slotId++ )
  {
    ItemMap::iterator it = m_itemMap.find( slotId );
    if( it == m_itemMap.end() ||
        it->second == nullptr )
      return slotId;
  }
  return -1;
}

Sapphire::ItemPtr Sapphire::ItemContainer::getItem( uint16_t slotId )
{

  if( slotId >= m_size )
  {
    Logger::error( "Slot out of range {} (maxSize={})", slotId, m_size );
    return nullptr;
  }

  const auto it = m_itemMap.find( slotId );
  if( it == m_itemMap.end() )
    return nullptr;

  return it->second;
}

void Sapphire::ItemContainer::setItem( uint16_t slotId, ItemPtr pItem )
{
  if( slotId >= m_size )
  {
    const uint32_t itemId = pItem ? pItem->getId() : 0;
    Logger::error( "Unable to place item {} at slot {} (maxSize={})", itemId, slotId, m_size );
    return;
  }

  if( !pItem )
  {
    m_itemMap.erase( slotId );
    return;
  }

  m_itemMap[ slotId ] = std::move( pItem );
}

uint16_t Sapphire::ItemContainer::getMaxSize() const
{
  return m_size > 65536 ? 65536 : m_size;
}

std::string Sapphire::ItemContainer::getTableName() const
{
  return m_tableName;
}

bool Sapphire::ItemContainer::isMultiStorage() const
{
  return m_bMultiStorage;
}

bool Sapphire::ItemContainer::isPersistentStorage() const
{
  return m_isPersistentStorage;
}
