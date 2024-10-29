// This is an automatically generated C++ script template
// Content needs to be added by hand to make it function
// In order for this script to be loaded, move it to the correct folder in <root>/scripts/

#include <Actor/Player.h>
#include "Manager/EventMgr.h"
#include <ScriptObject.h>
#include <Service.h>

// Quest Script: ClsLnc998_00132
// Quest Name: Way of the Lancer
// Quest ID: 65668
// Start NPC: 1000251 (Jillian)
// End NPC: 1000254 (Ywain)

using namespace Sapphire;

class ClsLnc998 : public Sapphire::ScriptAPI::QuestScript
{
  private:
    // Basic quest information 
    // Quest vars / flags used
    // UI8AL

    /// Countable Num: 1 Seq: 255 Event: 1 Listener: 1000254
    // Steps in this quest ( 0 is before accepting, 
    // 1 is first, 255 means ready for turning it in
    enum Sequence : uint8_t
    {
      Seq0 = 0,
      SeqFinish = 255,
    };

    // Entities found in the script data of the quest
    static constexpr auto Actor0 = 1000251; // Jillian ( Pos: 147.082001 15.501700 -267.993988  Teri: 133 )
    static constexpr auto Actor1 = 1000254; // Ywain ( Pos: 157.705002 15.884500 -270.326996  Teri: 133 )
    static constexpr auto Classjob = 4;
    static constexpr auto GearsetUnlock = 1905;
    static constexpr auto LogmessageMonsterNotePageUnlock = 1007;
    static constexpr auto UnlockImageClassLnc = 23;

  public:
    ClsLnc998() : Sapphire::ScriptAPI::QuestScript( 65668 ){}; 
    ~ClsLnc998() = default; 

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
          break;
        }
        case Actor1:
        {
          if( quest.getSeq() == SeqFinish )
            Scene00001( quest, player );
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
    eventMgr().playQuestScene( player, getId(), 0, NONE, bindSceneReturn( &ClsLnc998::Scene00000Return ) );
  }

  void Scene00000Return( World::Quest& quest, Entity::Player& player, const Event::SceneResult& result )
  {
    if( result.getResult( 0 ) == 1 ) // accept quest
    {
      quest.setSeq( SeqFinish );
    }


  }

  //////////////////////////////////////////////////////////////////////

  void Scene00001( World::Quest& quest, Entity::Player& player )
  {
    eventMgr().playQuestScene( player, getId(), 1, FADE_OUT | HIDE_UI, bindSceneReturn( &ClsLnc998::Scene00001Return ) );
  }

  void Scene00001Return( World::Quest& quest, Entity::Player& player, const Event::SceneResult& result )
  {

    if( result.getResult( 0 ) == 1 )
    {
      player.finishQuest( getId() );
      player.setLevelForClass( 1, Sapphire::Common::ClassJob::Lancer );
      player.addGearSet();
    }
  }
};

EXPOSE_SCRIPT( ClsLnc998 );