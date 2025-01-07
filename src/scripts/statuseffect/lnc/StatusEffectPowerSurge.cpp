#include <Action/CommonAction.h>
#include <Actor/Player.h>
#include <Script/NativeScriptApi.h>
#include <ScriptObject.h>
#include <StatusEffect/StatusEffect.h>

using namespace Sapphire;
using namespace Sapphire::World::Action;

class StatusEffectPowerSurge : public Sapphire::ScriptAPI::StatusEffectScript
{
public:
  StatusEffectPowerSurge() : Sapphire::ScriptAPI::StatusEffectScript( 120 )
  {
  }

  void onApply( Entity::Chara& actor ) override
  {
    // Placeholder implementation
  }
};

EXPOSE_SCRIPT( StatusEffectPowerSurge );