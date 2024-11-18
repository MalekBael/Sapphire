// This is an automatically generated C++ script template
// Content needs to be added by hand to make it function
// In order for this script to be loaded, move it to the correct folder in <root>/scripts/

#include "Manager/EventMgr.h"
#include <Actor/Player.h>
#include <Actor/BNpc.h>
#include <ScriptObject.h>
#include <Service.h>


// Quest Script: ClsLnc100_00218
// Quest Name: My First Spear
// Quest ID: 65754
// Start NPC: 1000254 (Ywain)
// End NPC: 1000254 (Ywain)

using namespace Sapphire;

class ClsLnc100 : public Sapphire::ScriptAPI::QuestScript
{
private:
  // Basic quest information
  // Quest vars / flags used
  // UI8AL
  // UI8BH
  // UI8BL

  /// Countable Num: 3 Seq: 1 Event: 5 Listener: 37
  /// Countable Num: 3 Seq: 1 Event: 5 Listener: 49
  /// Countable Num: 3 Seq: 1 Event: 5 Listener: 47
  /// Countable Num: 1 Seq: 255 Event: 1 Listener: 1000254
  // Steps in this quest ( 0 is before accepting,
  // 1 is first, 255 means ready for turning it in
  enum Sequence : uint8_t
  {
    Seq0 = 0,
    Seq1 = 1,
    SeqFinish = 255,
  };

  // Entities found in the script data of the quest
  static constexpr auto Actor0 = 1000254;// Ywain ( Pos: 157.705002 15.884500 -270.326996  Teri: 133 )
  static constexpr auto Enemy0 = 37;     // Ground Squirrel
  static constexpr auto Enemy1 = 49;     // Little Ladybug ( Pos: 230.500000 52.037998 138.003998  Teri: 140 )
  static constexpr auto Enemy2 = 47;     // Forest Funguar ( Pos: 263.598999 -6.623660 1.705660  Teri: 148 )
  static constexpr auto Seq0Actor0 = 0;  //
  static constexpr auto Seq2Actor0 = 1;  //

public:
  ClsLnc100() : Sapphire::ScriptAPI::QuestScript( 65754 ){};
  ~ClsLnc100() = default;

  //////////////////////////////////////////////////////////////////////
  // Event Handlers
  void onTalk( World::Quest& quest, Entity::Player& player, uint64_t actorId ) override
  {
    switch( actorId )
    {
      case Actor0:
      {
        if( quest.getSeq() == Seq0 )
          Scene00000( quest, player );
        else
          ( quest.getSeq() == Seq1 );
          Scene00001( quest, player );
        break;
      }
    }
  }

  void onEventItem( World::Quest& quest, Entity::Player& player, uint64_t actorId ) override
  {
  }

  void onBNpcKill( World::Quest& quest, Entity::BNpc& bnpc, Entity::Player& player ) override
  {
    switch( bnpc.getBNpcNameId() )
    {
      case Enemy0:
      {
        break;
      }
      case Enemy1:
      {
        break;
      }
      case Enemy2:
      {
        break;
      }
    }
  }

private:
  //////////////////////////////////////////////////////////////////////
  // Available Scenes in this quest, not necessarly all are used
  //////////////////////////////////////////////////////////////////////

  void Scene00000( World::Quest& quest, Entity::Player& player )
  {
    eventMgr().playQuestScene( player, getId(), 0, NONE, bindSceneReturn( &ClsLnc100::Scene00000Return ) );
  }

  void Scene00000Return( World::Quest& quest, Entity::Player& player, const Event::SceneResult& result )
  {
    if( result.getResult( 0 ) == 1 )// accept quest
    {
    }
  }

  //////////////////////////////////////////////////////////////////////

  void Scene00001( World::Quest& quest, Entity::Player& player )
  {
    eventMgr().playQuestScene( player, getId(), 1, NONE, bindSceneReturn( &ClsLnc100::Scene00001Return ) );
  }

  void Scene00001Return( World::Quest& quest, Entity::Player& player, const Event::SceneResult& result )
  {

    if( result.getResult( 0 ) == 1 )
    {
      player.setRewardFlag( Sapphire::Common::UnlockEntry::HuntingLog );
      player.finishQuest( getId(), result.getResult( 1 ) );
    }
  }
};

EXPOSE_SCRIPT( ClsLnc100 );