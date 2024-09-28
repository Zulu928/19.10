#include "die.h"
#include "gui.h"

void SetZoneToIndexHook(AFortGameModeAthena* GameModeAthena, int OverridePhaseMaybeIDFK)
{
	if (bStartedBus == true)
	{
		static auto ZoneDurationsOffset = Fortnite_Version >= 19.10 ? 0x258
			: 0x248;
		0x1F8; // S13-S14

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

		static int NewLateGameSafeZonePhase = 1;

		LOG_INFO(LogDev, "NewLateGameSafeZonePhase: {}", NewLateGameSafeZonePhase);

		if (Fortnite_Version < 12.41)
		{
			if (Globals::bLateGame.load())
			{
				GameModeAthena->Get<int>(GameMode_SafeZonePhaseOffset) = NewLateGameSafeZonePhase;
				GameState->Get<int>(GameState_SafeZonePhaseOffset) = NewLateGameSafeZonePhase;
				SetZoneToIndexOriginal(GameModeAthena, OverridePhaseMaybeIDFK);

				if (NewLateGameSafeZonePhase == EndReverseZonePhase) bZoneReversing = false;
				if (NewLateGameSafeZonePhase == 1 || NewLateGameSafeZonePhase == 2) SafeZoneIndicator->SkipShrinkSafeZone();
				if (NewLateGameSafeZonePhase >= StartReverseZonePhase) bZoneReversing = false;

				NewLateGameSafeZonePhase = (bZoneReversing && bEnableReverseZone) ? NewLateGameSafeZonePhase - 1 : NewLateGameSafeZonePhase + 1;

				return;
			}
			return SetZoneToIndexOriginal(GameModeAthena, OverridePhaseMaybeIDFK);
		}

		if (!SafeZoneIndicator)
		{
			LOG_WARN(LogZone, "Invalid SafeZoneIndicator!");
			return SetZoneToIndexOriginal(GameModeAthena, OverridePhaseMaybeIDFK);
		}

		static auto SafeZoneFinishShrinkTimeOffset = SafeZoneIndicator->GetOffset("SafeZoneFinishShrinkTime");
		static auto SafeZoneStartShrinkTimeOffset = SafeZoneIndicator->GetOffset("SafeZoneStartShrinkTime");
		static auto RadiusOffset = SafeZoneIndicator->GetOffset("Radius");
		static auto LocationOffset = SafeZoneIndicator->GetOffset("Location");


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

		int Phase = GameModeAthena->Get<int>(SafeZonePhaseOffset);
		int HoldDuration = 0;
		int ZoneDuration = 0;

		switch (Phase) {
		case 2:
			HoldDuration = (Phase >= 80 && Phase < ZoneHoldDurations.size()) ? ZoneHoldDurations.at(Phase) : 80.0f;
			ZoneDuration = (Phase >= 70 && Phase < ZoneDurations.size()) ? ZoneDurations.at(Phase) : 70.0f;
			break;
		case 3:
			HoldDuration = (Phase >= 50 && Phase < ZoneHoldDurations.size()) ? ZoneHoldDurations.at(Phase) : 50.0f;
			ZoneDuration = (Phase >= 60 && Phase < ZoneDurations.size()) ? ZoneDurations.at(Phase) : 60.0f;
			break;
		case 4:
			HoldDuration = (Phase >= 30 && Phase < ZoneHoldDurations.size()) ? ZoneHoldDurations.at(Phase) : 30.0f;
			ZoneDuration = (Phase >= 60 && Phase < ZoneDurations.size()) ? ZoneDurations.at(Phase) : 60.0f;
			break;
		case 5:
			HoldDuration = (Phase >= 30 && Phase < ZoneHoldDurations.size()) ? ZoneHoldDurations.at(Phase) : 30.0f;
			ZoneDuration = (Phase >= 60 && Phase < ZoneDurations.size()) ? ZoneDurations.at(Phase) : 60.0f;
			break;
		case 6:
			HoldDuration = (Phase >= 30 && Phase < ZoneHoldDurations.size()) ? ZoneHoldDurations.at(Phase) : 30.0f;
			ZoneDuration = (Phase >= 60 && Phase < ZoneDurations.size()) ? ZoneDurations.at(Phase) : 60.0f;
			break;
		case 7:
			HoldDuration = (Phase >= 0 && Phase < ZoneHoldDurations.size()) ? ZoneHoldDurations.at(Phase) : 0.0f;
			ZoneDuration = (Phase >= 60 && Phase < ZoneDurations.size()) ? ZoneDurations.at(Phase) : 55.0f;
			break;
		case 8:
			HoldDuration = (Phase >= 0 && Phase < ZoneHoldDurations.size()) ? ZoneHoldDurations.at(Phase) : 0.0f;
			ZoneDuration = (Phase >= 75 && Phase < ZoneDurations.size()) ? ZoneDurations.at(Phase) : 75.0f;
			break;
		case 9:
			HoldDuration = (Phase >= 0 && Phase < ZoneHoldDurations.size()) ? ZoneHoldDurations.at(Phase) : 0.0f;
			ZoneDuration = (Phase >= 80 && Phase < ZoneDurations.size()) ? ZoneDurations.at(Phase) : 80.0f;
			break;
		}

		float currentTime = GameState->GetServerWorldTimeSeconds();
		SafeZoneIndicator->Get<float>(SafeZoneStartShrinkTimeOffset) = currentTime + HoldDuration;

		if (ZoneDuration > 0) {
			SafeZoneIndicator->Get<float>(SafeZoneFinishShrinkTimeOffset) = SafeZoneIndicator->Get<float>(SafeZoneStartShrinkTimeOffset) + ZoneDuration;
		}

		//LOG_INFO(LogZone, "ZoneHoldDuration: {}", holdDuration);
		//LOG_INFO(LogZone, "ZoneDuration: {}", zoneDuration);



		auto GameMode = Cast<AFortGameModeAthena>(GetWorld()->GetGameMode());
		static auto SafeZoneLocationsOffset = GameMode->GetOffset("SafeZoneLocations");
		//	const TArray<FVector>& SafeZoneLocations = GameMode->Get<TArray<FVector>>(SafeZoneLocationsOffset);
			//const FVector ZoneCenterLocation = SafeZoneLocations.at(3);
		//	FVector LocationToStartAircraft = ZoneCenterLocation;

		//LOG_INFO(LogZone, "ZoneHoldDuration: {}", ZoneHoldDurations);
		//LOG_INFO(LogZone, "ZoneDuration: {}", ZoneDuration);
		LOG_INFO(LogZone, "Current Radius: {}", SafeZoneIndicator->Get<float>(RadiusOffset));
		LOG_INFO(LogZone, "Current SafeZoneLocation: {}", SafeZoneIndicator->Get<float>(SafeZoneLocationsOffset));

		int PlayersLeft = GameState->GetPlayersLeft();
		int safeZonePhase = GameModeAthena->Get<int>(SafeZonePhaseOffset);

		if (PlayersLeft >= 5)
		{
			if (safeZonePhase == 2)
			{
				LOG_INFO(LogDebug, "SafeZoneIndicatorNull1");
				LOG_WARN(LogZone, "SafeZoneIndicator is null during skip.");
				LOG_INFO(LogDebug, "SafeZoneIndicatorNull11");

				static auto RadiusOffset = SafeZoneIndicator->GetOffset("Radius");
				const float InitialSafeZoneRadius = 20002.766f;
				SafeZoneIndicator->Get<float>(RadiusOffset) = InitialSafeZoneRadius;
				LOG_INFO(LogDebug, "Get1");
				LOG_INFO(LogZone, "Initial Safe Zone Radius set to: {}", InitialSafeZoneRadius);
				LOG_INFO(LogDebug, "return SetZoneToIndexOriginal");

				SetZoneToIndexOriginal(GameModeAthena, OverridePhaseMaybeIDFK);

				return SetZoneToIndexOriginal(GameModeAthena, OverridePhaseMaybeIDFK);
			}

			if (safeZonePhase == 3) {
				static auto RadiusOffset = SafeZoneIndicator->GetOffset("Radius");
				const float InitialSafeZoneRadius = 10180.38f;
				SafeZoneIndicator->Get<float>(RadiusOffset) = InitialSafeZoneRadius;
				LOG_INFO(LogZone, "Current SafeZoneLocation: {}", SafeZoneIndicator->Get<float>(GameMode->GetOffset("SafeZoneLocations")));
				LOG_INFO(LogZone, "Initial Safe Zone Radius set to: {}", InitialSafeZoneRadius);

				SetZoneToIndexOriginal(GameModeAthena, OverridePhaseMaybeIDFK);

				return SetZoneToIndexOriginal(GameModeAthena, OverridePhaseMaybeIDFK);
			}

			if (safeZonePhase == 4 || safeZonePhase == 5) 
			{
				SafeZoneIndicator->SkipShrinkSafeZone();
				LOG_WARN(LogZone, "SafeZoneIndicator is null during skip.");
				static auto SafeZoneLocationsOffset = GameMode->GetOffset("SafeZoneLocations");
				LOG_INFO(LogZone, "Current SafeZoneLocation: {}", SafeZoneIndicator->Get<float>(SafeZoneLocationsOffset));

				static auto RadiusOffset = SafeZoneIndicator->GetOffset("Radius");
				float InitialSafeZoneRadius = (safeZonePhase == 4) ? 5000.38f : 2500.922f;
				SafeZoneIndicator->Get<float>(RadiusOffset) = InitialSafeZoneRadius;
				LOG_INFO(LogZone, "Initial Safe Zone Radius set to: {}", InitialSafeZoneRadius);
			}
		}
		else
		{
			if (PlayersLeft < 5)
			{
				if (safeZonePhase == 1 || safeZonePhase == 2)
				{
					SafeZoneIndicator->SkipShrinkSafeZone();
					LOG_WARN(LogZone, "SafeZoneIndicator is null during skip.");
					static auto SafeZoneLocationsOffset = GameMode->GetOffset("SafeZoneLocations");
					LOG_INFO(LogZone, "Current SafeZoneLocation: {}", SafeZoneIndicator->Get<float>(SafeZoneLocationsOffset));
				}

				if (safeZonePhase == 3)
				{
					static auto RadiusOffset = SafeZoneIndicator->GetOffset("Radius");
					const float InitialSafeZoneRadius = 10180.38f;
					SafeZoneIndicator->Get<float>(RadiusOffset) = InitialSafeZoneRadius;
					LOG_INFO(LogZone, "Current SafeZoneLocation: {}", SafeZoneIndicator->Get<float>(GameMode->GetOffset("SafeZoneLocations")));
					LOG_INFO(LogZone, "Initial Safe Zone Radius set to: {}", InitialSafeZoneRadius);

					SetZoneToIndexOriginal(GameModeAthena, OverridePhaseMaybeIDFK);

					return SetZoneToIndexOriginal(GameModeAthena, OverridePhaseMaybeIDFK);
				}

				if (safeZonePhase == 4 || safeZonePhase == 5)
				{
					SafeZoneIndicator->SkipShrinkSafeZone();
					LOG_WARN(LogZone, "SafeZoneIndicator is null during skip.");
					static auto SafeZoneLocationsOffset = GameMode->GetOffset("SafeZoneLocations");
					LOG_INFO(LogZone, "Current SafeZoneLocation: {}", SafeZoneIndicator->Get<float>(SafeZoneLocationsOffset));

					static auto RadiusOffset = SafeZoneIndicator->GetOffset("Radius");
					float InitialSafeZoneRadius = (safeZonePhase == 4) ? 5000.38f : 2500.922f;
					SafeZoneIndicator->Get<float>(RadiusOffset) = InitialSafeZoneRadius;
					LOG_INFO(LogZone, "Initial Safe Zone Radius set to: {}", InitialSafeZoneRadius);
				}
			}
		}
	}
}
