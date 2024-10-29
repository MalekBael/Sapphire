// This is an automatically generated C++ script template
// Content needs to be added by hand to make it function
// In order for this script to be loaded, move it to the correct folder in <root>/scripts/

#include <Actor/Player.h>
#include "Manager/EventMgr.h"
#include <ScriptObject.h>
#include <Service.h>

// Quest Script: ClsLnc999_00180
// Quest Name: So You Want to Be a Lancer
// Quest ID: 65716
// Start NPC: 1000251 (Jillian)
// End NPC: 1000251 (Jillian)

using namespace Sapphire;

class ClsLnc999 : public Sapphire::ScriptAPI::QuestScript
{
  private:
    // Basic quest information 
    // Quest vars / flags used

    // Steps in this quest ( 0 is before accepting, 
    // 1 is first, 255 means ready for turning it in
    enum Sequence : uint8_t
    {
    };

    // Entities found in the script data of the quest

    static constexpr auto Actor0 = 1000251;


  public:
    ClsLnc999() : Sapphire::ScriptAPI::QuestScript( 65716 ){}; 
    ~ClsLnc999() = default; 

  //////////////////////////////////////////////////////////////////////
  // Event Handlers
    void onTalk( World::Quest& quest, Entity::Player& player, uint64_t actorId ) override
    {
      switch( actorId )
      {
        case Actor0:
        {
          Scene00000( quest, player );
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
    eventMgr().playQuestScene( player, getId(), 0, HIDE_HOTBAR, bindSceneReturn( &ClsLnc999::Scene00000Return ) );
  }

  void Scene00000Return( World::Quest& quest, Entity::Player& player, const Event::SceneResult& result )
  {
    if( result.getResult( 0 ) == 1 )
    {
      player.finishQuest( getId(), 0 );
    }
  }
};
EXPOSE_SCRIPT( ClsLnc999 );