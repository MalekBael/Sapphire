#pragma once

#include <cstdint>
#include <vector>
#include <string>

namespace Sapphire::World::Action
{
  enum ActionSkill
  {
    SkullSunder = 35,
    Maim = 37,
    StormsPath = 42,
    StormsEye = 45,
    ButchersBlock = 47,
    // GLA
    Rampart = 10,
    FastBlade = 9,
    // ARC
    HeavyShot = 97,
    StraightShot = 98,
    VenomousBite = 100,
    RagingStrikes = 101,
    // LNC
    Feint = 76,
    KeenFlurry = 77,
    HeavyThrust = 79,
    LegSweep = 82,
    LifeSurge = 83,
    BloodForBlood = 85,
    ChaosThrust = 88,
    Phlebotomize = 91,
    PowerSurge = 93,
    ElusiveJump = 94,
    SpineshatterDive = 95,
    // Buffs
    Celeris = 404,
  };

  enum ActionStatus
  {
    Defiance = 91,
    Unchained = 92,
    Wrath = 93,
    WrathII = 94,
    WrathIII = 95,
    WrathIV = 96,
    Infuriated = 97,
    InnerBeast = 411,
    Deliverance = 729,
    // GLA
    Status_Rampart = 71,
    // ARC
    StraighterShot = 122,
    Venomous_Bite = 124,
    Raging_Strikes = 125,
    // LNC
    Heavy_Thrust = 115,
    Keen_Flurry = 114,
    Life_Surge = 116,
    Blood_For_Blood = 117,
    Chaos_Thrust = 118,
    Status_Phlebotomize = 119,
    Power_Surge = 120,
    Disembowel = 121,
    // Ailments
    Stun = 149,
    Slow = 561,
    // Buffs
    Haste = 651,
  };
}