#include <Action/CommonAction.h>
#include <Actor/Player.h>
#include <Script/NativeScriptApi.h>
#include <ScriptObject.h>
#include <StatusEffect/StatusEffect.h>

using namespace Sapphire;
using namespace Sapphire::World::Action;

class StatusEffectStun : public Sapphire::ScriptAPI::StatusEffectScript
{
public:
  StatusEffectStun() : Sapphire::ScriptAPI::StatusEffectScript( 149 )
  {
  }

  void onApply( Entity::Chara& actor ) override
  {
    // Placeholder implementation
  }
};

EXPOSE_SCRIPT( StatusEffectStun );