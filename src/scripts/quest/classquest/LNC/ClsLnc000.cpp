// This is an automatically generated C++ script template
// Content needs to be added by hand to make it function
// In order for this script to be loaded, move it to the correct folder in <root>/scripts/

#include "Manager/EventMgr.h"
#include <Actor/Player.h>
#include <Actor/BNpc.h>
#include <ScriptObject.h>
#include <Service.h>

// Quest Script: ClsLnc000_00023
// Quest Name: Way of the Lancer
// Quest ID: 65559
// Start NPC: 1000251 (Jillian)
// End NPC: 1000254 (Ywain)

using namespace Sapphire;

class ClsLnc000 : public Sapphire::ScriptAPI::QuestScript
{
  private:
    // Basic quest information 
    // Quest vars / flags used
    // UI8AL
    // UI8BH
    // UI8BL

    /// Countable Num: 1 Seq: 1 Event: 1 Listener: 1000254
    /// Countable Num: 3 Seq: 2 Event: 5 Listener: 37
    /// Countable Num: 3 Seq: 2 Event: 5 Listener: 49
    /// Countable Num: 3 Seq: 2 Event: 5 Listener: 47
    /// Countable Num: 1 Seq: 255 Event: 1 Listener: 1000254
    // Steps in this quest ( 0 is before accepting, 
    // 1 is first, 255 means ready for turning it in
    enum Sequence : uint8_t
    {
      Seq0 = 0,
      Seq1 = 1,
      Seq2 = 2,
      SeqFinish = 255,
    };

    // Entities found in the script data of the quest
    static constexpr auto Actor0 = 1000251;// Jillian ( Pos: 147.082001 15.501700 -267.993988  Teri: 133 )
    static constexpr auto Actor1 = 1000254;// Ywain ( Pos: 157.705002 15.884500 -270.326996  Teri: 133 )
    static constexpr auto Enemy0 = 37;     // Ground Squirrel
    static constexpr auto Enemy1 = 49;     // Little Ladybug ( Pos: 230.500000 52.037998 138.003998  Teri: 140 )
    static constexpr auto Enemy2 = 47;     // Forest Funguar ( Pos: 263.598999 -6.623660 1.705660  Teri: 148 )
    static constexpr auto LogmessageMonsterNotePageUnlock = 1007;
    static constexpr auto Seq0Actor0 = 0;//
    static constexpr auto Seq1Actor1 = 1;//
    static constexpr auto Seq3Actor1 = 2;// Ruins Runner ( Pos: -5.462710 -1.142520 27.215000  Teri: 5 )
    static constexpr auto UnlockImageMonsterNote = 32;


  public:
    ClsLnc000() : Sapphire::ScriptAPI::QuestScript( 65559 ){}; 
    ~ClsLnc000() = default; 

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
        if( quest.getSeq() == Seq1 )
          Scene00001( quest, player );
        else
          Scene00002( quest, player );
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
      case Enemy0: { break; }
      case Enemy1: { break; }
      case Enemy2: { break; }
    }
  }

  private:
  //////////////////////////////////////////////////////////////////////
  // Available Scenes in this quest, not necessarly all are used
  //////////////////////////////////////////////////////////////////////

  void Scene00000( World::Quest& quest, Entity::Player& player )
  {
    eventMgr().playQuestScene( player, getId(), 0, NONE, bindSceneReturn( &ClsLnc000::Scene00000Return ) );
  }

  void Scene00000Return( World::Quest& quest, Entity::Player& player, const Event::SceneResult& result )
  {
    if( result.getResult( 0 ) == 1 ) // accept quest
    {
      quest.setSeq( Seq1 );
 
    }


  }

  //////////////////////////////////////////////////////////////////////

  void Scene00001( World::Quest& quest, Entity::Player& player )
  {
    eventMgr().playQuestScene( player, getId(), 1, FADE_OUT | HIDE_UI, bindSceneReturn( &ClsLnc000::Scene00001Return ) );
  }

  void Scene00001Return( World::Quest& quest, Entity::Player& player, const Event::SceneResult& result )
  {


  }

  //////////////////////////////////////////////////////////////////////

  void Scene00002( World::Quest& quest, Entity::Player& player )
  {
    eventMgr().playQuestScene( player, getId(), 2, HIDE_HOTBAR, bindSceneReturn( &ClsLnc000::Scene00002Return ) );
  }

  void Scene00002Return( World::Quest& quest, Entity::Player& player, const Event::SceneResult& result )
  {

    if( result.getResult( 0 ) == 1 )
    {
      player.setRewardFlag( Sapphire::Common::UnlockEntry::HuntingLog );
      player.finishQuest( getId(), result.getResult( 1 ) );
    }
  }
};

EXPOSE_SCRIPT( ClsLnc000 );