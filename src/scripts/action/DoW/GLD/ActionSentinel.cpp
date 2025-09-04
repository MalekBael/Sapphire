#include <Script/NativeScriptApi.h>
#include <ScriptObject.h>
#include <Actor/Chara.h>
#include <Action/CommonAction.h>
#include <Action/Action.h>
#include <StatusEffect/StatusEffect.h>
#include <Math/CalcStats.h>
#include <Actor/BNpc.h>
#include <Util/UtilMath.h>

using namespace Sapphire;
using namespace Sapphire::World::Action;

class ActionSentinel : public Sapphire::ScriptAPI::ActionScript
{
public:
  ActionSentinel() : Sapphire::ScriptAPI::ActionScript( Sentinel )
  {
  }

  static constexpr auto Duration = 10;
  static constexpr uint32_t Flags = static_cast< uint32_t >( Common::StatusEffectFlag::BuffCategory );

  void onExecute( Sapphire::World::Action::Action& action ) override
  {
    auto pSource = action.getSourceChara();
    auto pActionBuilder = action.getActionResultBuilder();

    if( pSource->getClass() == Common::ClassJob::Gladiator && pSource->getLevel() >= 48 )// Todo: check for parity
    {}// Todo: lv48 sets effect to 40%

    pActionBuilder->applyStatusEffectSelf( SentinelStatus, ( Duration * 1000 ), 0, {}, Flags, true );
  }
};

EXPOSE_SCRIPT( ActionSentinel );