#include "die.h"
#include "gui.h"

void SetZoneToIndexHook(AFortGameModeAthena* GameModeAthena, int OverridePhaseMaybeIDFK)
{
    static auto ZoneDurationsOffset = Fortnite_Version >= 15 && Fortnite_Version < 18 ? 0x258
        : std::floor(Fortnite_Version) >= 18 ? 0x248
        : 0x1F8; // S13-S14

    static int32 GameMode_SafeZonePhaseOffset = 0;
    static int32 GameState_SafeZonePhaseOffset = 0;
    static bool bFilledDurations = false;

    static auto SafeZonePhaseOffset = GameModeAthena->GetOffset("SafeZonePhase");

    auto GameState = Cast<AFortGameStateAthena>(GameModeAthena->GetGameState());

    if (!GameState)
    {
        LOG_ERROR(LogZone, "Invalid GameState!");
        return SetZoneToIndexOriginal(GameModeAthena, OverridePhaseMaybeIDFK);
    }

    auto SafeZoneIndicator = GameModeAthena->GetSafeZoneIndicator();
    if (!SafeZoneIndicator)
    {
        LOG_ERROR(LogZone, "Invalid SafeZoneIndicator!");
        return SetZoneToIndexOriginal(GameModeAthena, OverridePhaseMaybeIDFK);
    }

    GameMode_SafeZonePhaseOffset = GameModeAthena->GetOffset("SafeZonePhase");
    GameState_SafeZonePhaseOffset = GameState->GetOffset("SafeZonePhase");
    SafeZonePhaseOffset = GameModeAthena->GetOffset("SafeZonePhase");

    const int MinSafeZonePhase = 0;
    const int MaxSafeZonePhase = 10;

    static int NewLateGameSafeZonePhase = (Fortnite_Version < 19) ? 1 : 1; 

    LOG_INFO(LogDev, "NewLateGameSafeZonePhase: {}", NewLateGameSafeZonePhase);

    if (Fortnite_Version < 19)
    {
        if (Globals::bLateGame.load())
        {
            if (NewLateGameSafeZonePhase != -1)
            {
                GameModeAthena->Get<int>(GameMode_SafeZonePhaseOffset) = NewLateGameSafeZonePhase;
                GameState->Get<int>(GameState_SafeZonePhaseOffset) = NewLateGameSafeZonePhase;
                SetZoneToIndexOriginal(GameModeAthena, OverridePhaseMaybeIDFK);

                if (NewLateGameSafeZonePhase == EndReverseZonePhase) bZoneReversing = false;
                if (NewLateGameSafeZonePhase == 2 || NewLateGameSafeZonePhase == 3) SafeZoneIndicator->SkipShrinkSafeZone();
                if (NewLateGameSafeZonePhase >= StartReverseZonePhase) bZoneReversing = false;

                NewLateGameSafeZonePhase = (bZoneReversing && bEnableReverseZone) ? NewLateGameSafeZonePhase - 1 : NewLateGameSafeZonePhase + 1;
            }
            else
            {
                SetZoneToIndexOriginal(GameModeAthena, OverridePhaseMaybeIDFK);
            }
            return;
        }
        SetZoneToIndexOriginal(GameModeAthena, OverridePhaseMaybeIDFK);
        return;
    }

    if (!SafeZoneIndicator)
    {
        LOG_WARN(LogZone, "Invalid SafeZoneIndicator!");
        return SetZoneToIndexOriginal(GameModeAthena, OverridePhaseMaybeIDFK);
    }

    static auto SafeZoneFinishShrinkTimeOffset = SafeZoneIndicator->GetOffset("SafeZoneFinishShrinkTime");
    static auto SafeZoneStartShrinkTimeOffset = SafeZoneIndicator->GetOffset("SafeZoneStartShrinkTime");
    static auto RadiusOffset = SafeZoneIndicator->GetOffset("Radius");

    static auto MapInfoOffset = GameState->GetOffset("MapInfo");
    auto MapInfo = GameState->Get<AActor*>(MapInfoOffset);

    if (!MapInfo)
    {
        LOG_WARN(LogZone, "Invalid MapInfo!");
        return SetZoneToIndexOriginal(GameModeAthena, OverridePhaseMaybeIDFK);
    }

    static auto SafeZoneDefinitionOffset = MapInfo->GetOffset("SafeZoneDefinition");
    auto SafeZoneDefinition = MapInfo->GetPtr<__int64>(SafeZoneDefinitionOffset);

    LOG_INFO(LogDev, "SafeZoneDefinitionOffset: 0x{:x}", SafeZoneDefinitionOffset);

    static auto ZoneHoldDurationsOffset = ZoneDurationsOffset - 0x10;

    auto& ZoneDurations = *(TArray<float>*)(__int64(SafeZoneDefinition) + ZoneDurationsOffset);
    auto& ZoneHoldDurations = *(TArray<float>*)(__int64(SafeZoneDefinition) + ZoneHoldDurationsOffset);

    if (!bFilledDurations)
    {
        bFilledDurations = true;

        auto CurrentPlaylist = GameState->GetCurrentPlaylist();
        UCurveTable* FortGameData = nullptr;

        static auto GameDataOffset = CurrentPlaylist->GetOffset("GameData");
        FortGameData = CurrentPlaylist ? CurrentPlaylist->Get<TSoftObjectPtr<UCurveTable>>(GameDataOffset).Get() : nullptr;

        if (!FortGameData)
            FortGameData = FindObject<UCurveTable>(L"/Game/Balance/AthenaGameData.AthenaGameData");

        if (!FortGameData)
        {
            LOG_ERROR(LogZone, "Unable to get FortGameData.");
            return SetZoneToIndexOriginal(GameModeAthena, OverridePhaseMaybeIDFK);
        }

        auto ShrinkTimeFName = UKismetStringLibrary::Conv_StringToName(L"Default.SafeZone.ShrinkTime");
        auto HoldTimeFName = UKismetStringLibrary::Conv_StringToName(L"Default.SafeZone.WaitTime");

        for (int i = 0; i < ZoneDurations.Num(); i++)
            ZoneDurations.at(i) = FortGameData->GetValueOfKey(FortGameData->GetKey(ShrinkTimeFName, i));

        for (int i = 0; i < ZoneHoldDurations.Num(); i++)
            ZoneHoldDurations.at(i) = FortGameData->GetValueOfKey(FortGameData->GetKey(HoldTimeFName, i));
    }

    LOG_INFO(LogZone, "SafeZonePhase: {}", GameModeAthena->Get<int>(SafeZonePhaseOffset));
    LOG_INFO(LogZone, "OverridePhaseMaybeIDFK: {}", OverridePhaseMaybeIDFK);
    LOG_INFO(LogZone, "TimeSeconds: {}", UGameplayStatics::GetTimeSeconds(GetWorld()));

    if (Globals::bLateGame.load())
    {
        if (NewLateGameSafeZonePhase != -1)
        {
            GameModeAthena->Get<int>(GameMode_SafeZonePhaseOffset) = NewLateGameSafeZonePhase;
            GameState->Get<int>(GameState_SafeZonePhaseOffset) = NewLateGameSafeZonePhase;
            SetZoneToIndexOriginal(GameModeAthena, OverridePhaseMaybeIDFK);

            if (NewLateGameSafeZonePhase == EndReverseZonePhase) bZoneReversing = false;
            if (NewLateGameSafeZonePhase >= StartReverseZonePhase) bZoneReversing = false;

            NewLateGameSafeZonePhase = (bZoneReversing && bEnableReverseZone) ? NewLateGameSafeZonePhase - 1 : NewLateGameSafeZonePhase + 1;
        }
        else
        {
            SetZoneToIndexOriginal(GameModeAthena, OverridePhaseMaybeIDFK);
        }
    }
    else
    {
        SetZoneToIndexOriginal(GameModeAthena, OverridePhaseMaybeIDFK);
    }

    LOG_INFO(LogZone, "SafeZonePhase After: {}", GameModeAthena->Get<int>(SafeZonePhaseOffset));

    float ZoneHoldDuration = 0.0f;
    int CurrentPhase = GameModeAthena->Get<int>(SafeZonePhaseOffset);
    if (CurrentPhase >= 0 && CurrentPhase < ZoneHoldDurations.Num())
    {
        ZoneHoldDuration = ZoneHoldDurations.at(CurrentPhase);
    }
    else
    {
        LOG_ERROR(LogZone, "Invalid SafeZonePhase: {}", CurrentPhase);
    }

    SafeZoneIndicator->Get<float>(SafeZoneStartShrinkTimeOffset) = GameState->GetServerWorldTimeSeconds() + ZoneHoldDuration;

    float ZoneDuration = 0.0f;
    if (CurrentPhase >= 0 && CurrentPhase < ZoneDurations.Num())
    {
        ZoneDuration = ZoneDurations.at(CurrentPhase);
    }
    else
    {
        LOG_ERROR(LogZone, "Invalid SafeZonePhase: {}", CurrentPhase);
    }

    LOG_INFO(LogZone, "ZoneDuration: {}", ZoneDuration);
    LOG_INFO(LogZone, "Current Radius: {}", SafeZoneIndicator->Get<float>(RadiusOffset));

    SafeZoneIndicator->Get<float>(SafeZoneFinishShrinkTimeOffset) = SafeZoneIndicator->Get<float>(SafeZoneStartShrinkTimeOffset) + ZoneDuration;

    if (CurrentPhase == 0)
    {
        const float FixedInitialZoneSize = 5000.0f;
        SafeZoneIndicator->Get<float>(RadiusOffset) = FixedInitialZoneSize;
        LOG_INFO(LogZone, "Initial Storm Zone Size set to: {}", FixedInitialZoneSize);
    }

    if (CurrentPhase == 2 || CurrentPhase == 3)
    {
        if (SafeZoneIndicator)
        {
            SafeZoneIndicator->SkipShrinkSafeZone();
        }
        else
        {
            LOG_WARN(LogZone, "SafeZoneIndicator is null during skip.");
        }
        UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), L"skipsafezone", nullptr);
    }
}
