#include "Logging/Logger.h"
#include <Actor/Player.h>
#include <Exd/ExdData.h>
#include <ScriptObject.h>

#include "Manager/MgrUtil.h"
#include "Manager/PlayerMgr.h"

using namespace Sapphire;

class NewScript : public Sapphire::ScriptAPI::EventScript
{
public:
  NewScript() : Sapphire::ScriptAPI::EventScript( 0x00200000 )
  {
    Manager::PlayerMgr::sendDebug(( "NewScript: Initialized" );// Simple initialization log
  }

  void onTalk( uint32_t eventId, Entity::Player& player, uint64_t actorId ) override
  {
    Manager::PlayerMgr::sendDebug(( "NewScript: Processing talk event {0} for actor {1}", eventId, actorId );

    auto& exdData = Common::Service< Data::ExdData >::ref();
    auto switchTalk = exdData.getRow< Excel::SwitchTalk >( eventId );
    uint32_t talkEvent = 0;

    if( !switchTalk )
    {
      Manager::PlayerMgr::sendDebug( "NewScript: Failed to get SwitchTalk data for event {0}", eventId );
      return;
    }

    for( auto entry = 15; entry >= 0; entry-- )
    {
      auto caseCondition = switchTalk->data().TalkCase[ entry ].CaseCondition;

      if( ( caseCondition >> 16 ) == Event::EventHandler::EventHandlerType::Quest && player.isQuestCompleted( caseCondition ) )
      {
        talkEvent = switchTalk->data().TalkCase[ entry ].Talk;
        Logger::debug( "NewScript: Found matching quest condition at entry {0}, talk event {1}", entry, talkEvent );
        break;
      }
      else
      {
        talkEvent = switchTalk->data().TalkCase[ 0 ].Talk;
      }
    }

    if( talkEvent == 0 )
    {
      Logger::debug( "NewScript: No valid talk event found for event {0}", eventId );
      return;
    }

    Logger::debug( "NewScript: Starting talk event {0}", talkEvent );
    eventMgr().eventStart( player, actorId, talkEvent, Event::EventHandler::EventType::Nest, 0, 5 );
    eventMgr().playScene( player, talkEvent, 0, HIDE_HOTBAR | NO_DEFAULT_CAMERA, { 0 }, nullptr );
  }
};

EXPOSE_SCRIPT( NewScript );
