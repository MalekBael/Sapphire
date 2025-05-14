// This is an automatically generated C++ script template
// Content needs to be added by hand to make it function
// In order for this script to be loaded, move it to the correct folder in <root>/scripts/

#include "Manager/EventMgr.h"
#include "ScriptLogger.h"
#include <Actor/Player.h>
#include <Event/EventHandler.h>
#include <ScriptObject.h>
#include <Service.h>

// Quest Script: SubFst070_00220
// Quest Name: Leves of Bentbranch
// Quest ID: 65756
// Start NPC: 1000101 (Gontrant)
// End NPC: 1000105 (Tierney)

using namespace Sapphire;

class SubFst070 : public Sapphire::ScriptAPI::QuestScript
{
private:
  // Basic quest information
  // Quest vars / flags used
  // UI8AL - Used to track if player has talked to Tierney
  // UI8BL - Used to track if player has seen levequest window

  enum Sequence : uint8_t
  {
    Seq0 = 0,       // Initial
    Seq1 = 1,       // Talk to Tierney
    Seq2 = 2,       // Return to Gontrant
    SeqFinish = 255,// Ready to complete
  };

  // Entities found in the script data of the quest
  static constexpr auto Actor0 = 1000101;     // Gontrant
  static constexpr auto Actor1 = 1000105;     // Tierney
  static constexpr auto GlAssignment = 393217;// 0x60001 - GuildleveAssignment event ID
  static constexpr auto Leve = 546;           // Target leve ID
  static constexpr auto UnlockImageLeve = 30;
  static constexpr auto HOWTO_ID_LEVE2 = 50;

  // Standard scene flags
  static constexpr auto FLAG_HIDE_HOTBAR = 0x2;
  static constexpr auto FLAG_SET_BASE = 0x20;
  static constexpr auto NONE = 0;

  std::string getNowTimeStr()
  {
    auto now = std::chrono::system_clock::now();
    auto timer = std::chrono::system_clock::to_time_t( now );
    std::tm bt;
    localtime_s( &bt, &timer );
    char buf[ 64 ];
    strftime( buf, sizeof( buf ), "%H:%M:%S", &bt );
    return std::string( buf );
  }

public:
  SubFst070() : Sapphire::ScriptAPI::QuestScript( 65756 ) {};
  ~SubFst070() = default;

  //////////////////////////////////////////////////////////////////////
  // Event Handlers
  void onTalk( World::Quest& quest, Entity::Player& player, uint64_t actorId ) override
  {
    switch( actorId )
    {
      case Actor0:// Gontrant
      {
        if( quest.getSeq() == Seq0 )
          Scene00000( quest, player );
        else if( quest.getSeq() == Seq2 )
          Scene00005( quest, player );
        else
          Scene00006( quest, player );
        break;
      }
      case Actor1:// Tierney
      {
        if( quest.getSeq() == Seq0 )
        {
          Scene00008( quest, player );
        }
        else if( quest.getSeq() == Seq1 )
        {
          if( quest.getUI8AL() == 0 )
          {
            ScriptAPI::ScriptLogger::debug( "[{}] First talk with Tierney", getNowTimeStr() );

            // Set flag to prevent repeat triggering
            quest.setUI8AL( 1 );

            // FIRST: Trigger the system event (GuildleveAssignment)
            ScriptAPI::ScriptLogger::debug( "[{}] Triggering SYSTEM EVENT: {}", getNowTimeStr(), GlAssignment );

            // Start the system event first - CRUCIAL: use player.getId() as the eventArg parameter!
            eventMgr().eventStart( player, Actor1, GlAssignment,
                                   static_cast< Event::EventHandler::EventType >( Event::EventHandler::Talk ),
                                   0, player.getId() );

            // Play scene 3 to show the leve list
            std::vector< uint32_t > params = { 1, Leve, 1, 0 };
            eventMgr().playScene(
                    player,
                    GlAssignment,
                    3,
                    FLAG_HIDE_HOTBAR | FLAG_SET_BASE,
                    params );

            // Show quest dialogue
            Scene00001( quest, player );
          }
          else if( quest.getUI8BL() == 0 )
          {
            // Player has talked to Tierney once but hasn't seen tutorial
            quest.setUI8BL( 1 );
            Scene00002( quest, player );
          }
          else
          {
            // Return dialogue
            Scene00003( quest, player );
          }
        }
        else if( quest.getSeq() == Seq2 )
        {
          Scene00007( quest, player );
        }
        else if( quest.getSeq() == SeqFinish )
        {
          Scene00004( quest, player );
        }
        else
        {
          Scene00003( quest, player );
        }
        break;
      }
    }
  }

  void onEventItem( World::Quest& quest, Entity::Player& player, uint64_t actorId ) override
  {
    // No event items in this quest
  }

private:
  // Basic scene handlers
  void Scene00000( World::Quest& quest, Entity::Player& player )
  {
    eventMgr().playQuestScene( player, getId(), 0, NONE, bindSceneReturn( &SubFst070::Scene00000Return ) );
  }

  void Scene00000Return( World::Quest& quest, Entity::Player& player, const Event::SceneResult& result )
  {
    if( result.getResult( 0 ) == 1 )// Player accepts quest
      quest.setSeq( 1 );
  }

  void Scene00001( World::Quest& quest, Entity::Player& player )
  {
    eventMgr().playQuestScene( player, getId(), 1, NONE, bindSceneReturn( &SubFst070::Scene00001Return ) );
  }

  void Scene00001Return( World::Quest& quest, Entity::Player& player, const Event::SceneResult& result )
  {
    // Handle returns from either the quest dialogue or the leve window
    if( result.eventId == GlAssignment )
    {
      // This is a return from the GuildleveAssignment event (levequest window)
      ScriptAPI::ScriptLogger::debug( "[{}] GuildleveAssignment event return: option={}, leveId={}",
                                      getNowTimeStr(), result.getResult( 0 ), result.getResult( 1 ) );

      // result.getResult(0) == 1 means the player accepted a leve
      // result.getResult(1) contains the leveId that was selected
      if( result.getResult( 0 ) == 1 && result.getResult( 1 ) == Leve )
      {
        ScriptAPI::ScriptLogger::debug( "[{}] Player accepted leve {}, progressing quest",
                                        getNowTimeStr(), Leve );

        // The player has selected the correct leve, set UI8BL flag
        quest.setUI8BL( 1 );

        // The player will now need to talk to Tierney again for the tutorial
      }
    }
    else
    {
      // This is a return from the regular quest scene
      ScriptAPI::ScriptLogger::debug( "[{}] Regular quest scene return", getNowTimeStr() );
    }
  }

  void Scene00002( World::Quest& quest, Entity::Player& player )
  {
    eventMgr().playQuestScene( player, getId(), 2, NONE, bindSceneReturn( &SubFst070::Scene00002Return ) );
  }

  void Scene00002Return( World::Quest& quest, Entity::Player& player, const Event::SceneResult& result )
  {
    // Show HowTo notification
    eventMgr().eventActionStart(
            player,
            getId(),
            HOWTO_ID_LEVE2,
            nullptr, nullptr, 0 );

    // Advance quest
    quest.setSeq( Seq2 );

    // Tell player to go back to Gontrant
    eventMgr().sendEventNotice( player, getId(), 1, 0, 0, 0 );
  }

  void Scene00003( World::Quest& quest, Entity::Player& player )
  {
    eventMgr().playQuestScene( player, getId(), 3, NONE, bindSceneReturn( &SubFst070::Scene00003Return ) );
  }

  void Scene00003Return( World::Quest& quest, Entity::Player& player, const Event::SceneResult& result )
  {
    // Generic greeting from Tierney
  }

  void Scene00004( World::Quest& quest, Entity::Player& player )
  {
    eventMgr().playQuestScene( player, getId(), 4, NONE, bindSceneReturn( &SubFst070::Scene00004Return ) );
  }

  void Scene00004Return( World::Quest& quest, Entity::Player& player, const Event::SceneResult& result )
  {
    if( result.getResult( 0 ) == 1 )
    {
      player.finishQuest( getId(), result.getResult( 1 ) );
      eventMgr().sendEventNotice( player, getId(), 0, 0, 0, 0 );
      eventMgr().eventActionStart( player, getId(), UnlockImageLeve, nullptr, nullptr, 0 );
    }
  }

  void Scene00005( World::Quest& quest, Entity::Player& player )
  {
    eventMgr().playQuestScene( player, getId(), 5, NONE, bindSceneReturn( &SubFst070::Scene00005Return ) );
  }

  void Scene00005Return( World::Quest& quest, Entity::Player& player, const Event::SceneResult& result )
  {
    quest.setSeq( SeqFinish );
  }

  void Scene00006( World::Quest& quest, Entity::Player& player )
  {
    eventMgr().playQuestScene( player, getId(), 6, NONE, bindSceneReturn( &SubFst070::Scene00006Return ) );
  }

  void Scene00006Return( World::Quest& quest, Entity::Player& player, const Event::SceneResult& result )
  {
    // Generic dialogue
  }

  void Scene00007( World::Quest& quest, Entity::Player& player )
  {
    eventMgr().playQuestScene( player, getId(), 7, NONE, bindSceneReturn( &SubFst070::Scene00007Return ) );
  }

  void Scene00007Return( World::Quest& quest, Entity::Player& player, const Event::SceneResult& result )
  {
    ScriptAPI::ScriptLogger::debug( "[{}] Opening levequest window again", getNowTimeStr() );

    // Start the system event with player ID as the third parameter
    eventMgr().eventStart( player, Actor1, GlAssignment,
                           static_cast< Event::EventHandler::EventType >( Event::EventHandler::Talk ),
                           0, player.getId() );

    // Show window
    std::vector< uint32_t > params = { 1, Leve, 1, 0 };
    eventMgr().playScene(
            player,
            GlAssignment,
            3,
            FLAG_HIDE_HOTBAR | FLAG_SET_BASE,
            params );
  }

  void Scene00008( World::Quest& quest, Entity::Player& player )
  {
    eventMgr().playQuestScene( player, getId(), 8, NONE, bindSceneReturn( &SubFst070::Scene00008Return ) );
  }

  void Scene00008Return( World::Quest& quest, Entity::Player& player, const Event::SceneResult& result )
  {
    // Generic dialogue
  }
};

EXPOSE_SCRIPT( SubFst070 );
