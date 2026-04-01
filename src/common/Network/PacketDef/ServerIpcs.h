#pragma once

#include <stdint.h>

namespace Sapphire::Network::Packets
{
  namespace LobbyPackets
  {
    ////////////////////////////////////////////////////////////////////////////////
    /// Lobby Connection IPC Codes
    /**
    * Server IPC Lobby Type Codes.
    */
    enum ServerLobbyIpcType : uint16_t
    {
      // Legacy (kept)
      SyncReply = 0x0001,

      NackReply = 0x0002, // outdated (7.45h2: LobbyError)
      LoginReply = 0x000C, // outdated (7.45h2: LobbyServiceAccountList)
      ServiceLoginReply = 0x000D, // outdated (7.45h2: LobbyCharList)
      CharaMakeReply = 0x000E, // outdated (7.45h2: LobbyCharCreate)
      GameLoginReply = 0x000F, // outdated (7.45h2: LobbyEnterWorld)

      UpdateRetainerSlotsReply = 0x0010,

      DistWorldInfo = 0x0015, // outdated (7.45h2: LobbyServerList)
      XiCharacterInfo = 0x0016,
      DistRetainerInfo = 0x0017, // outdated (7.45h2: LobbyRetainerList)

      DebugNullRepl = 0x01F4,
      DebugLoginRepl = 0x01F5,

      // 7.45h2 (takes precedence)
      LobbyError = 0x0002,
      LobbyServiceAccountList = 0x000C,
      LobbyCharList = 0x000D,
      LobbyCharCreate = 0x000E,
      LobbyEnterWorld = 0x000F,
      LobbyServerList = 0x0015,
      LobbyRetainerList = 0x0017,
    };

    /**
    * Client IPC Lobby Type Codes.
    */
    enum ClientLobbyIpcType : uint16_t
    {
      // 7.45h2
      ReqCharList = 0x0003,
      ReqEnterWorld = 0x0004,
      ClientVersionInfo = 0x0005,

      ReqCharDelete = 0x000A,
      ReqCharCreate = 0x000B,
    };
  }

  namespace WorldPackets::Server
  {
    ////////////////////////////////////////////////////////////////////////////////
    /// Zone Connection IPC Codes
    /**
    * Server IPC Zone Type Codes.
    */
    enum ServerZoneIpcType : uint16_t
    {
      // ---- Legacy list (kept; conflicts marked outdated) ----
      SyncReply = 0x0065,
      LoginReply = 0x0066,
      ChatToChannel = 0x0067,
      RegionInfo = 0x0069,

      MoveTerritory = 0x006A,
      MoveInstance = 0x006B,

      SetPSNId = 0x0073, // outdated (7.45h2: EventPlay32)
      SetBillingTime = 0x0075, // outdated (7.45h2: AirshipStatus)

      AllienceInviteResult = 0x0078,
      AllienceInviteReplyResult = 0x0079,
      AllienceInviteUpdate = 0x007A,

      InviteResult = 0x00C9,
      InviteReplyResult = 0x00CA,
      InviteUpdate = 0x00CB,
      GetCommonlistResult = 0x00CC,
      GetCommonlistDetailResult = 0x00CD,
      SetProfileResult = 0x00CE,
      GetProfileResult = 0x00CF,

      GetSearchCommentResult = 0x00D0,
      GetCharacterNameResult = 0x00D1,
      ChatChannelResult = 0x00D2,
      SendSystemMessage = 0x00D3,
      SendLoginMessage = 0x00D4, // outdated (7.45h2: SystemLogMessage)
      UpdateOnlineStatus = 0x00D5,
      PartyRecruitResult = 0x00D6,
      GetRecruitSearchListResult = 0x00D7,
      GetRecruitDetailResult = 0x00D8,
      RequestItemResult = 0x00D9,
      AllianceReadyCheckResult = 0x00DA,
      GetFcJoinRequestCommentResult = 0x00DB,
      PcPartyResult = 0x00DC,
      PcPartyUpdate = 0x00DD,
      InviteCancelResult = 0x00DE,

      BlacklistAddResult = 0x00E1,
      BlacklistRemoveResult = 0x00E2,
      GetBlacklistResult = 0x00E3,

      FriendlistRemoveResult = 0x00E6,

      PcSearchResult = 0x00EB,

      LinkshellResult = 0x00F0,
      GetLinkshellListResult = 0x00F1,

      LetterResult = 0x00FA,
      GetLetterMessageResult = 0x00FB,
      GetLetterMessageDetailResult = 0x00FC,
      GetLetterStatusResult = 0x00FD,

      ItemSearchResult = 0x0104,
      GetItemSearchListResult = 0x0105,
      GetRetainerListResult = 0x0106,
      BuyMarketRetainerResult = 0x0107,
      MarketStorageUpdate = 0x0108,
      GetItemHistoryResult = 0x0109, // outdated (7.45h2: PlayerSpawn)
      GetRetainerSalesHistoryResult = 0x010A,
      MarketRetainerUpdate = 0x010B,
      CatalogSearchResult = 0x010C,

      FreeCompanyResult = 0x010E,
      GetFcStatusResult = 0x010F,
      GetFcInviteListResult = 0x0110,
      GetFcProfileResult = 0x0111,
      GetFcHeaderResult = 0x0112,
      GetCompanyBoardResult = 0x0113,
      GetFcHierarchyResult = 0x0114,
      GetFcActivityListResult = 0x0115,
      GetFcHierarchyLiteResult = 0x0116,
      GetCompanyMottoResult = 0x0117,
      GetFcParamsResult = 0x0118,
      GetFcActionResult = 0x0119,
      GetFcMemoResult = 0x011A,

      LeaveNoviceNetwork = 0x011D,

      InfoGMCommandResult = 0x0122,

      SyncTagHeader = 0x012C,
      SyncTag32 = 0x012D,
      SyncTag64 = 0x012E,
      SyncTag128 = 0x012F,
      SyncTag256 = 0x0130,
      SyncTag384 = 0x0131,
      SyncTag512 = 0x0132,
      SyncTag768 = 0x0133,
      SyncTag1024 = 0x0134,
      SyncTag1536 = 0x0135,
      SyncTag2048 = 0x0136,
      SyncTag3072 = 0x0137,

      HudParam = 0x0140,
      ActionIntegrity = 0x0141,
      Order = 0x0142, // outdated (7.45h2: CraftingLog)
      OrderMySelf = 0x0143,
      OrderTarget = 0x0144,
      Resting = 0x0145,
      ActionResult1 = 0x0146,
      ActionResult = 0x0147,
      Status = 0x0148,
      FreeCompany = 0x0149,
      RecastGroup = 0x014A,
      UpdateAlliance = 0x014B,
      PartyPos = 0x014C,
      AlliancePos = 0x014D,

      GrandCompany = 0x014F,

      Create = 0x0190,
      Delete = 0x0191,
      ActorMove = 0x0192, // outdated (7.45h2: ActorMove)
      Transfer = 0x0193,
      Warp = 0x0194,

      RequestCast = 0x0196,

      UpdateParty = 0x0199,
      InitZone = 0x019A, // outdated (7.45h2: InitZone)
      HateList = 0x019B,
      HaterList = 0x019C,
      CreateObject = 0x019D,
      DeleteObject = 0x019E,
      PlayerStatusUpdate = 0x019F,
      PlayerStatus = 0x01A0, // outdated (7.45h2: PrepareZoning)
      BaseParam = 0x01A1, // outdated (7.45h2: UpdateClassInfo)
      FirstAttack = 0x01A2,
      Condition = 0x01A3,
      ChangeClass = 0x01A4,
      Equip = 0x01A5,
      Inspect = 0x01A6,
      Name = 0x01A7,

      AttachMateriaRequest = 0x01A9,
      RetainerList = 0x01AA, // outdated (7.45h2: FreeCompanyInfo)
      RetainerData = 0x01AB,
      MarketPriceHeader = 0x01AC,
      MarketPrice = 0x01AD,
      ItemStorage = 0x01AE,
      NormalItem = 0x01AF,
      ItemSize = 0x01B0,
      ItemOperationBatch = 0x01B1,
      ItemOperation = 0x01B2,
      GilItem = 0x01B3,
      TradeCommand = 0x01B4, // outdated (7.45h2: CFNotify)
      ItemMessage = 0x01B5,
      UpdateItem = 0x01B6,
      AliasItem = 0x01B7,
      OpenTreasure = 0x01B8,
      LootRight = 0x01B9,
      LootActionResult = 0x01BA,
      GameLog = 0x01BB,
      TreasureOpenRight = 0x01BC,
      OpenTreasureKeyUi = 0x01BD,
      LootItems = 0x01BE,
      CreateTreasure = 0x01BF, // outdated (7.45h2: EventFinish)
      TreasureFadeOut = 0x01C0,
      MonsterNoteCategory = 0x01C1,
      EventPlayHeader = 0x01C2,
      EventPlay2 = 0x01C3,
      EventPlay4 = 0x01C4, // outdated (7.45h2: UpdateHpMpTp)
      EventPlay8 = 0x01C5,
      EventPlay16 = 0x01C6,
      EventPlay32 = 0x01C7,
      EventPlay64 = 0x01C8,
      EventPlay128 = 0x01C9,
      EventPlay255 = 0x01CA,
      DebugActorData = 0x01CB,
      PushEventState = 0x01CC,
      PopEventState = 0x01CD,
      UpdateEventSceneHeader = 0x01CE,
      UpdateEventScene2 = 0x01CF,
      UpdateEventScene4 = 0x01D0,
      UpdateEventScene8 = 0x01D1,
      UpdateEventScene16 = 0x01D2,
      UpdateEventScene32 = 0x01D3, // outdated (7.45h2: ExamineSearchInfo)
      UpdateEventScene64 = 0x01D4,
      UpdateEventScene128 = 0x01D5,
      UpdateEventScene255 = 0x01D6,
      ResumeEventSceneHeader = 0x01D7,
      ResumeEventScene2 = 0x01D8,
      ResumeEventScene4 = 0x01D9,
      ResumeEventScene8 = 0x01DA,
      ResumeEventScene16 = 0x01DB,
      ResumeEventScene32 = 0x01DC,
      ResumeEventScene64 = 0x01DD,
      ResumeEventScene128 = 0x01DE,
      ResumeEventScene255 = 0x01DF,
      Quests = 0x01E0,
      Quest = 0x01E1,
      QuestCompleteFlags = 0x01E2,
      QuestCompleteFlag = 0x01E3,
      Guildleves = 0x01E4,
      Guildleve = 0x01E5,
      LeveCompleteFlags = 0x01E6,
      LeveCompleteFlag = 0x01E7,
      NoticeHeader = 0x01E8,
      Notice2 = 0x01E9,
      Notice4 = 0x01EA,
      Notice8 = 0x01EB,
      Notice16 = 0x01EC,
      Notice32 = 0x01ED,
      Tracking = 0x01EE,
      IsMarket = 0x01EF,
      LegacyQuestCompleteFlags = 0x01F0,
      ResumeEventSceneHeaderStr = 0x01F1,
      ResumeEventSceneStr32 = 0x01F2,
      LogText = 0x01F3,
      DebugNull = 0x01F4,
      DebugLog = 0x01F5,
      BigData = 0x01F6,
      DebugOrderHeader = 0x01F7,
      DebugOrder2 = 0x01F8,
      DebugOrder4 = 0x01F9,
      DebugOrder8 = 0x01FA,
      DebugOrder16 = 0x01FB,
      DebugOrder32 = 0x01FC,
      DebugActionRange = 0x01FD,
      ResumeEventSceneHeaderNumStr = 0x01FE,
      ResumeEventScene2Str = 0x01FF,
      Mount = 0x0200,
      ResumeEventScene4Str = 0x0201,

      Director = 0x0226,

      EventLogMessageHeader = 0x0258,
      EventLogMessage2 = 0x0259,
      EventLogMessage4 = 0x025A,
      EventLogMessage8 = 0x025B,
      EventLogMessage16 = 0x025C,
      EventLogMessage32 = 0x025D,

      BattleTalkHeader = 0x0262,
      BattleTalk2 = 0x0263,
      BattleTalk4 = 0x0264, // outdated (7.45h2: UpdateHpMpTp)
      BattleTalk8 = 0x0265,
      EventReject = 0x026C,
      MapMarker2 = 0x026D,
      MapMarker4 = 0x026E, // outdated (7.45h2: ObjectSpawn)
      MapMarker8 = 0x026F,
      MapMarker16 = 0x0270,
      MapMarker32 = 0x0271, // outdated (7.45h2: Examine)
      MapMarker64 = 0x0272,
      MapMarker128 = 0x0273,
      BalloonTalkHeader = 0x0276, // outdated (7.45h2: DesynthResult)
      BalloonTalk2 = 0x0277,
      BalloonTalk4 = 0x0278,
      BalloonTalk8 = 0x0279,

      GameLoggerMessage = 0x0289,
      WeatherId = 0x028A, // outdated (7.45h2: ItemMarketBoardInfo)
      TitleList = 0x028B,
      DiscoveryReply = 0x028C, // outdated (7.45h2: SubmarineExplorationResult)
      TimeOffset = 0x028D,
      ChocoboTaxiStart = 0x028E,
      GMOrderHeader = 0x028F,
      GMOrder2 = 0x0290,
      GMOrder4 = 0x0291,
      GMOrder8 = 0x0292,
      GMOrder16 = 0x0293,
      GMOrder32 = 0x0294, // outdated (7.45h2: EventPlay8)

      InspectQuests = 0x029E,
      InspectGuildleves = 0x029F,
      InspectReward = 0x02A0,
      InspectBeastReputation = 0x02A1,

      Config = 0x02C6,

      NpcYell = 0x02D0,
      SwapSystem = 0x02D1,
      FatePcWork = 0x02D2,
      LootResult = 0x02D3,
      FateAccessCollectionEventObject = 0x02D4,
      FateSyncLimitTime = 0x02D5,
      EnableLogout = 0x02D6,
      LogMessage = 0x02D7,
      FateDebug = 0x02D8,
      FateContextWork = 0x02D9,
      FateActiveRange = 0x02DA,
      UpdateFindContent = 0x02DB,
      Cabinet = 0x02DC,
      Achievement = 0x02DD,
      NotifyFindContentStatus = 0x02DE,
      ColosseumResult44 = 0x02DF,
      ColosseumResult88 = 0x02E0, // outdated (7.45h2: UpdateInventorySlot)
      ResponsePenalties = 0x02E1,
      ContentClearFlags = 0x02E2,
      ContentAttainFlags = 0x02E3,
      UpdateContent = 0x02E4,
      Text = 0x02E5,
      SuccessFindContentAsMember5 = 0x02E6,
      CancelLogoutCountdown = 0x02E7,
      SetEventBehavior = 0x02E8,
      BallistaStart = 0x02E9,
      RetainerCustomizePreset = 0x02EA, // outdated (7.45h2: EnvironmentControl)
      FateReward = 0x02EB,

      HouseList = 0x02EC,
      House = 0x02ED, // outdated (7.45h2: AirshipExplorationResult)
      YardObjectList = 0x02EE,

      YardObject = 0x02F0,
      Interior = 0x02F1,
      HousingAuction = 0x02F2, // outdated (7.45h2: InitZone)
      HousingProfile = 0x02F3,
      HousingHouseName = 0x02F4,
      HousingGreeting = 0x02F5,
      CharaHousingLandData = 0x02F6,
      CharaHousing = 0x02F7,
      HousingWelcome = 0x02F8,
      FurnitureListS = 0x02F9,
      FurnitureListM = 0x02FA,
      FurnitureListL = 0x02FB,
      Furniture = 0x02FC,
      VoteKickConfirm = 0x02FD, // outdated (7.45h2: InventoryTransactionFinish)
      HousingProfileList = 0x02FE, // outdated (7.45h2: EventPlay)
      HousingObjectTransform = 0x02FF,
      HousingObjectColor = 0x0300,
      HousingObjectTransformMulti = 0x0301,
      ConfusionApproach = 0x0302, // outdated (7.45h2: WeatherChange)
      ConfusionTurn = 0x0303,
      ConfusionTurnCancel = 0x0304,
      ConfusionCancel = 0x0305, // outdated (7.45h2: MarketBoardPurchase)
      MIPMemberList = 0x0306,
      HousingGetPersonalRoomProfileListResult = 0x0307,
      HousingGetHouseBuddyStableListResult = 0x0308,
      HouseTrainBuddyData = 0x0309,

      ContentBonus = 0x0311, // outdated (7.45h2: ActorControl)

      FcChestLog = 0x0316,
      SalvageResult = 0x0317,

      DailyQuests = 0x0320,
      DailyQuest = 0x0321,
      QuestRepeatFlags = 0x0322,

      HousingObjectTransformMultiResult = 0x032A,
      HousingLogWithHouseName = 0x032B,
      TreasureHuntReward = 0x032C,
      HousingCombinedObjectStatus = 0x032D,
      HouseBuddyModelData = 0x032E,

      Marker = 0x0334,
      GroundMarker = 0x0335,
      Frontline01Result = 0x0336,
      Frontline01BaseInfo = 0x0337,

      FinishContentMatchToClient = 0x0339,

      UnMountLink = 0x033E,

      // ---- 7.45h2 (takes precedence) ----
      PlayerSetup = 0x008E, // updated 7.45h2
      UpdateHpMpTp = 0x0264, // updated 7.45h2
      UpdateClassInfo = 0x00A1, // updated 7.45h2
      PlayerStats = 0x02AA, // updated 7.45h2
      ActorControl = 0x0311, // updated 7.45h2
      ActorControlSelf = 0x03C9, // updated 7.45h2
      ActorControlTarget = 0x0171, // updated 7.45h2
      Playtime = 0x016D, // updated 7.45h2
      UpdateSearchInfo = 0x017F, // updated 7.45h2
      ExamineSearchInfo = 0x01D3, // updated 7.45h2
      Examine = 0x0271, // updated 7.45h2
      ActorCast = 0x03B9, // updated 7.45h2
      CurrencyCrystalInfo = 0x02CF, // updated 7.45h2
      InitZone = 0x02F2, // updated 7.45h2
      WeatherChange = 0x0302, // updated 7.45h2
      PlayerSpawn = 0x0109, // updated 7.45h2
      ActorSetPos = 0x0085, // updated 7.45h2
      PrepareZoning = 0x01A0, // updated 7.45h2
      ContainerInfo = 0x027D, // updated 7.45h2
      ItemInfo = 0x0159, // updated 7.45h2
      PlaceFieldMarker = 0x0101, // updated 7.45h2
      PlaceFieldMarkerPreset = 0x023C, // updated 7.45h2
      EffectResult = 0x0186, // updated 7.45h2
      EventStart = 0x0181, // updated 7.45h2
      EventFinish = 0x01BF, // updated 7.45h2
      DesynthResult = 0x0276, // updated 7.45h2
      FreeCompanyInfo = 0x01AA, // updated 7.45h2
      FreeCompanyDialog = 0x03DE, // updated 7.45h2
      MarketBoardSearchResult = 0x0172, // updated 7.45h2
      MarketBoardItemListingCount = 0x006F, // updated 7.45h2
      MarketBoardItemListingHistory = 0x00F2, // updated 7.45h2
      MarketBoardItemListing = 0x022D, // updated 7.45h2
      MarketBoardPurchase = 0x0305, // updated 7.45h2
      UpdateInventorySlot = 0x02E0, // updated 7.45h2
      InventoryActionAck = 0x00AD, // updated 7.45h2
      InventoryTransaction = 0x02BB, // updated 7.45h2
      InventoryTransactionFinish = 0x02FD, // updated 7.45h2
      ResultDialog = 0x025F, // updated 7.45h2
      RetainerInformation = 0x0286, // updated 7.45h2
      NpcSpawn = 0x0328, // updated 7.45h2
      ItemMarketBoardInfo = 0x028A, // updated 7.45h2
      ObjectSpawn = 0x026E, // updated 7.45h2
      EffectResultBasic = 0x027C, // updated 7.45h2
      Effect = 0x02A4, // updated 7.45h2
      StatusEffectList = 0x0234, // updated 7.45h2
      StatusEffectList2 = 0x038C, // updated 7.45h2
      StatusEffectList3 = 0x0388, // updated 7.45h2
      ActorGauge = 0x03D8, // updated 7.45h2
      CFNotify = 0x01B4, // updated 7.45h2
      SystemLogMessage = 0x00D4, // updated 7.45h2
      AirshipTimers = 0x00C3, // updated 7.45h2
      SubmarineTimers = 0x007F, // updated 7.45h2
      AirshipStatusList = 0x02A7, // updated 7.45h2
      AirshipStatus = 0x0075, // updated 7.45h2
      AirshipExplorationResult = 0x02ED, // updated 7.45h2
      SubmarineProgressionStatus = 0x0236, // updated 7.45h2
      SubmarineStatusList = 0x02A5, // updated 7.45h2
      SubmarineExplorationResult = 0x028C, // updated 7.45h2

      CraftingLog = 0x0142, // updated 7.45h2
      GatheringLog = 0x0387, // updated 7.45h2

      ActorMove = 0x02B0, // updated 7.45h2

      EventPlay = 0x02FE, // updated 7.45h2
      EventPlay4 = 0x024C, // updated 7.45h2
      EventPlay8 = 0x0294, // updated 7.45h2
      EventPlay16 = 0x0340, // updated 7.45h2
      EventPlay32 = 0x0073, // updated 7.45h2
      EventPlay64 = 0x03C8, // updated 7.45h2
      EventPlay128 = 0x0331, // updated 7.45h2
      EventPlay255 = 0x016C, // updated 7.45h2

      EnvironmentControl = 0x02EA, // updated 7.45h2
      IslandWorkshopSupplyDemand = 0x009B, // updated 7.45h2
      Logout = 0x031E, // updated 7.45h2
    };
  }

  ////////////////////////////////////////////////////////////////////////////////
  /// Zone Connection IPC Codes
  /**
  * Client IPC Zone Type Codes.
  */
  enum ClientZoneIpcType : uint16_t
  {
    UpdatePositionHandler = 0x010E, // updated 7.45h2
    SetSearchInfoHandler = 0x027A, // updated 7.45h2
    MarketBoardPurchaseHandler = 0x01D1, // updated 7.45h2
    InventoryModifyHandler = 0x0249, // updated 7.45h2
  };

  ////////////////////////////////////////////////////////////////////////////////
  /// Chat Connection IPC Codes
  /**
  * Server IPC Chat Type Codes.
  */
  enum ServerChatIpcType : uint16_t
  {
    //Sync = 0x01, TODO: Fix this cuz name bullshit...
    LoginReply = 0x02,

    ChatFrom = 0x64,
    Chat = 0x65,
    TellNotFound = 0x66,
    RecvBusyStatus = 0x67,
    GetChannelMemberListResult = 0x68,
    GetChannelListResult = 0x69,
    RecvFinderStatus = 0x6A,

    JoinChannelResult = 0xC8,
    LeaveChannelResult = 0xC9
    //FreeCompanyEvent = 0x012C, // not in 2.3
  };

  /**
  * Client IPC Chat Type Codes.
  */
  enum ClientChatIpcType : uint16_t
  {
    //TellReq = 0x0064,
  };
}
