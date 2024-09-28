#include "BuildingActor.h"

#include "FortWeapon.h"
#include "BuildingSMActor.h"
#include "FortPlayerControllerAthena.h"
#include "FortPawn.h"
#include "FortWeaponMeleeItemDefinition.h"
#include "CurveTable.h"
#include "DataTable.h"
#include "FortResourceItemDefinition.h"
#include "FortKismetLibrary.h"
#include "DataTableFunctionLibrary.h"
#include "BuildingContainer.h"

void ABuildingActor::OnDamageServerHook(ABuildingActor* BuildingActor, float Damage, FGameplayTagContainer DamageTags,
	FVector Momentum, /* FHitResult */ __int64 HitInfo, APlayerController* InstigatedBy, AActor* DamageCauser,
	/* FGameplayEffectContextHandle */ __int64 EffectContext)
{
	//LOG_INFO(LogDev, "OnDamageServerHook1");

	auto BuildingSMActor = Cast<ABuildingSMActor>(BuildingActor);
	if (auto Container = Cast<ABuildingContainer>(BuildingActor))
	{
		if ((BuildingSMActor->GetHealth() <= 0 || BuildingActor->GetHealth() <= 0) && !Container->IsAlreadySearched())
		{
			LOG_INFO(LogDev, "It's a me, a buildingcontainer!");
			Container->SpawnLoot();
		}
	}
	auto AttachedBuildingActors = BuildingSMActor->GetAttachedBuildingActors();
	for (int i = 0; i < AttachedBuildingActors.Num(); ++i)
	{

		auto CurrentBuildingActor = AttachedBuildingActors.at(i);
		//LOG_INFO(LogDev, "item = {}", CurrentBuildingActor->GetFullName());
		auto CurrentActor = Cast<ABuildingActor>(CurrentBuildingActor);
		if (BuildingSMActor->GetHealth() <= 0 || BuildingActor->GetHealth() <= 0)
		{
			//LOG_INFO(LogDev, "BuildingSMActor currentHealth {}", BuildingSMActor->GetHealth());
			//LOG_INFO(LogDev, "BuildingActor currentHealth {}", BuildingActor->GetHealth());
			//LOG_INFO(LogDev, "CurrentBuildingActor currentHealth {}", CurrentBuildingActor->GetHealth());
			//LOG_INFO(LogDev, "CurrentActor currentHealth {}", CurrentActor->GetHealth());
			if (auto Container = Cast<ABuildingContainer>(CurrentActor))
			{
				if (!Container->IsAlreadySearched())
				{
					Container->SpawnLoot();
					LOG_INFO(LogDev, "spawnloott");
				}
			}
		}
	}
	//auto builds = Cast<TArray<ABuildingActor>>(AttachedBuildingActors);
	//ReceiveDestroyedHook(BuildingActor);
	auto PlayerController = Cast<AFortPlayerControllerAthena>(InstigatedBy);
	// auto Pawn = PlayerController ? PlayerController->GetMyFortPawn() : nullptr;
	auto Weapon = Cast<AFortWeapon>(DamageCauser);
	//LOG_INFO(LogDev, "bolbol");
	if (!BuildingSMActor)
		return OnDamageServerOriginal(BuildingActor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);


	if (BuildingSMActor->IsDestroyed())
		return OnDamageServerOriginal(BuildingActor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);

	/*

	static auto LastDamageAmountOffset = BuildingSMActor->GetOffset("LastDamageAmount");
	static auto LastDamageHitOffset = BuildingSMActor->GetOffset("LastDamageHit", false) != -1 ? BuildingSMActor->GetOffset("LastDamageHit") : BuildingSMActor->GetOffset("LastDamageHitImpulseDir"); // idc

	const float PreviousLastDamageAmount = BuildingSMActor->Get<float>(LastDamageAmountOffset);
	const float PreviousLastDamageHit = BuildingSMActor->Get<float>(LastDamageHitOffset);
	const float CurrentBuildingHealth = BuildingActor->GetHealth();

	BuildingSMActor->Get<float>(LastDamageAmountOffset) = Damage;
	BuildingSMActor->Get<float>(LastDamageHitOffset) = CurrentBuildingHealth;

	*/

	if (!PlayerController || !Weapon)
		return OnDamageServerOriginal(BuildingActor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);

	// if (!Pawn)
		// return OnDamageServerOriginal(BuildingActor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);

	auto WorldInventory = PlayerController->GetWorldInventory();

	if (!WorldInventory)
		return OnDamageServerOriginal(BuildingActor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);

	auto WeaponData = Cast<UFortWeaponMeleeItemDefinition>(Weapon->GetWeaponData());

	if (!WeaponData)
		return OnDamageServerOriginal(BuildingActor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);

	UFortResourceItemDefinition* ItemDef = UFortKismetLibrary::K2_GetResourceItemDefinition(BuildingSMActor->GetResourceType());

	if (!ItemDef)
		return OnDamageServerOriginal(BuildingActor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);

	static auto BuildingResourceAmountOverrideOffset = BuildingSMActor->GetOffset("BuildingResourceAmountOverride");
	auto& BuildingResourceAmountOverride = BuildingSMActor->Get<FCurveTableRowHandle>(BuildingResourceAmountOverrideOffset);

	int ResourceCount = 0;


	if (BuildingResourceAmountOverride.RowName.IsValid())
	{
		// auto AssetManager = Cast<UFortAssetManager>(GEngine->AssetManager);
		//auto GameState = Cast<AFortGameStateAthena>(GetWorld()->GetGameState());
		UCurveTable* CurveTable = nullptr; //GameState->GetCurrentPlaylist().BasePlaylist ? GameState->GetCurrentPlaylist().BasePlaylist->ResourceRates.Get() : nullptr;

		// LOG_INFO(LogDev, "Before1");

		if (!CurveTable)
			CurveTable = FindObject<UCurveTable>(L"/Game/Athena/Balance/DataTables/AthenaResourceRates.AthenaResourceRates");

		{
			// auto curveMap = ((UDataTable*)CurveTable)->GetRowMap();

			// LOG_INFO(LogDev, "Before {}", __int64(CurveTable));

			float Out = UDataTableFunctionLibrary::EvaluateCurveTableRow(CurveTable, BuildingResourceAmountOverride.RowName, 0.f);

			// LOG_INFO(LogDev, "Out: {}", Out);

			const float DamageThatWillAffect = /* PreviousLastDamageHit > 0 && Damage > PreviousLastDamageHit ? PreviousLastDamageHit : */ Damage;

			//LOG_INFO(LogDev, "Out {}", Out);

			//LOG_INFO(LogDev, "maxHealth {}", BuildingActor->GetMaxHealth());

			//LOG_INFO(LogDev, "damageThatWill {}", DamageThatWillAffect);

			//LOG_INFO(LogDev, "currentHealth {}", BuildingActor->GetHealth());

			float skid = Out / (BuildingActor->GetMaxHealth() / DamageThatWillAffect);

			ResourceCount = round(skid);
		}
	}

	if (ResourceCount <= 0)
	{
		return OnDamageServerOriginal(BuildingActor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
	}

	bool bIsWeakspot = Damage == 100.0f;
	PlayerController->ClientReportDamagedResourceBuilding(BuildingSMActor, BuildingSMActor->GetResourceType(), ResourceCount, false, bIsWeakspot);
	bool bShouldUpdate = false;
	int MaximumMaterial = Globals::MaxMats;
	auto MatDefinition = UFortKismetLibrary::K2_GetResourceItemDefinition(BuildingSMActor->GetResourceType());
	auto MatInstance = WorldInventory->FindItemInstance(MatDefinition);
	if (!MatInstance) {
		WorldInventory->AddItem(ItemDef, &bShouldUpdate, ResourceCount);
		if (bShouldUpdate)
			WorldInventory->Update();
		return OnDamageServerOriginal(BuildingActor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
	}
	auto MatsAfterUpdate = MatInstance->GetItemEntry()->GetCount() + ResourceCount;
	FGuid MatsGUID = MatInstance->GetItemEntry()->GetItemGuid();
	WorldInventory->AddItem(ItemDef, &bShouldUpdate, ResourceCount);
	if (MatsAfterUpdate > MaximumMaterial)
	{
		auto Pawn = PlayerController->GetMyFortPawn();
		PickupCreateData CreateData;
		CreateData.ItemEntry = FFortItemEntry::MakeItemEntry(ItemDef, MatsAfterUpdate - MaximumMaterial, -1, MAX_DURABILITY);
		CreateData.SpawnLocation = Pawn->GetActorLocation();
		CreateData.PawnOwner = Cast<AFortPawn>(Pawn);
		CreateData.SourceType = EFortPickupSourceTypeFlag::GetPlayerValue();
		CreateData.bShouldFreeItemEntryWhenDeconstructed = true;

		AFortPickup::SpawnPickup(CreateData);
		WorldInventory->FindReplicatedEntry(MatsGUID)->GetCount() = MaximumMaterial;
	}

	if (bShouldUpdate)
		WorldInventory->Update();

	return OnDamageServerOriginal(BuildingActor, Damage, DamageTags, Momentum, HitInfo, InstigatedBy, DamageCauser, EffectContext);
}

UClass* ABuildingActor::StaticClass()
{
	static auto Class = FindObject<UClass>(L"/Script/FortniteGame.BuildingActor");
	return Class;
}
