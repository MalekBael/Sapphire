#include "ActorFilter.h"
#include "Actor/GameObject.h"
#include "Util/Util.h"
#include "Util/UtilMath.h"


Sapphire::World::Util::ActorFilterInRange::ActorFilterInRange( Common::FFXIVARR_POSITION3 startPos,
                                                               float range ) : m_startPos( startPos ),
                                                                               m_range( range )
{
}

bool Sapphire::World::Util::ActorFilterInRange::conditionApplies( const Entity::GameObject& actor )
{
  // Get distance from AOE center to actor position
  float distance = Sapphire::Common::Util::distance( m_startPos, actor.getPos() );

  // Apply a range multiplier to match the visual telegraph size
  // For CircularAOEWithPadding (cast type 5), we need this to be accurate
  constexpr float AOE_RANGE_MULTIPLIER = 1.3f; // Adjust based on testing
  float effectiveRange = m_range * AOE_RANGE_MULTIPLIER;
  
  // Add player hitbox consideration - players have approximately 0.5 yalm radius
  if( actor.isPlayer() )
  {
    constexpr float PLAYER_HITBOX_RADIUS = 0.5f;
    
    // Debug logging (remove or comment out in production)
    // Logger::debug( "Player at distance {:.2f}, effective range {:.2f} + hitbox {:.2f}",
    //                distance, effectiveRange, PLAYER_HITBOX_RADIUS );
    
    // If any part of the player hitbox is inside the AOE radius, they get hit
    return distance <= ( effectiveRange + PLAYER_HITBOX_RADIUS );
  }
  
  // For other entities, use the adjusted range without hitbox consideration
  return distance <= effectiveRange;
}


///////////////////////////////////////////////////////////////////////////////////////////////////////

Sapphire::World::Util::ActorFilterSingleTarget::ActorFilterSingleTarget( uint32_t actorId ) : m_actorId( actorId )
{
}

bool Sapphire::World::Util::ActorFilterSingleTarget::conditionApplies( const Sapphire::Entity::GameObject& actor )
{
  return actor.getId() == m_actorId;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

Sapphire::World::Util::ActorFilterRectangle::ActorFilterRectangle( Common::FFXIVARR_POSITION3 sourcePos,
                                                                   float rotation,
                                                                   float length,
                                                                   float width,
                                                                   uint32_t actorId ) : m_rotation( rotation ),
                                                                                        m_length( length ),
                                                                                        m_width( width )
{
  m_sourcePos.x = sourcePos.x;
  m_sourcePos.y = sourcePos.y;
  m_sourcePos.z = sourcePos.z;
  m_sourcePos.actorId = actorId;
}

bool Sapphire::World::Util::ActorFilterRectangle::conditionApplies( const Entity::GameObject& actor )
{
  // Skip self check - prevent source from being hit by its own AOE
  if( actor.getId() == m_sourcePos.actorId && m_sourcePos.actorId != 0 )
    return false;

  // First, translate the actor's position relative to the source position
  float relX = actor.getPos().x - m_sourcePos.x;
  float relZ = actor.getPos().z - m_sourcePos.z;

  // Rotate the point to align with the rectangle's orientation
  float rotatedX = relX * cosf( -m_rotation ) - relZ * sinf( -m_rotation );
  float rotatedZ = relX * sinf( -m_rotation ) + relZ * cosf( -m_rotation );

  // Apply the wiki formula for rectangular AOEs: 6y + 120 degrees
  // Length is based on the effect range provided (equivalent to 6y in the wiki formula)
  float length = m_length;

  // Width calculation based on 120 degrees angle
  // For a 120-degree cone at distance length, width = 2 * length * tan(60°)
  // tan(60°) ≈ 1.732, so width ≈ 3.464 * length
  constexpr float CONE_WIDTH_FACTOR = 3.464f;// 2 * tan(60°)

  // Use the larger of: the provided width × multiplier, or the calculated cone width
  float coneBasedWidth = length * CONE_WIDTH_FACTOR;
  float configuredWidth = m_width * 2.5f;// Keep the existing multiplier for backward compatibility

  float adjustedWidth = std::max( configuredWidth, coneBasedWidth );
  float halfWidth = adjustedWidth / 2.0f;

  // Now check if the rotated point is within the rectangle bounds
  // Rectangle extends from (0, -halfWidth) to (length, halfWidth)
  return ( rotatedX >= 0 && rotatedX <= length &&
           rotatedZ >= -halfWidth && rotatedZ <= halfWidth );
}


///////////////////////////////////////////////////////////////////////////////////////////////////////
