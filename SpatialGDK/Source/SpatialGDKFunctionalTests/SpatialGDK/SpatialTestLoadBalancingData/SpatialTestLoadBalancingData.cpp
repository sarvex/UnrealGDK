// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "SpatialGDK/SpatialTestLoadBalancingData/SpatialTestLoadBalancingData.h"

#include "EngineClasses/SpatialNetDriver.h"
#include "EngineClasses/SpatialPackageMapClient.h"

#include "SpatialCommonTypes.h"
#include "SpatialConstants.h"

#include "EngineClasses/SpatialWorldSettings.h"
#include "Kismet/GameplayStatics.h"

void USpatialTestLoadBalancingDataTestMap::CreateCustomContentForMap()
{
	Super::CreateCustomContentForMap();

	Cast<ASpatialWorldSettings>(World->GetWorldSettings())
		->SetMultiWorkerSettingsClass(USpatialTestLoadBalancingDataMultiWorkerSettings::StaticClass());

	AddActorToLevel<ASpatialTestLoadBalancingData>(World->GetCurrentLevel(), FTransform::Identity);
}

USpatialTestLoadBalancingDataMultiWorkerSettings::USpatialTestLoadBalancingDataMultiWorkerSettings()
{
	WorkerLayers[0].ActorClasses.Add(ASpatialTestLoadBalancingDataActor::StaticClass());
	WorkerLayers.Add(
		{ TEXT("Offloaded"), { ASpatialTestLoadBalancingDataOffloadedActor::StaticClass() }, USingleWorkerStrategy::StaticClass() });
}

ASpatialTestLoadBalancingDataActor::ASpatialTestLoadBalancingDataActor()
{
	bReplicates = true;

	bAlwaysRelevant = true;
}

ASpatialTestLoadBalancingDataOffloadedActor::ASpatialTestLoadBalancingDataOffloadedActor()
{
	bReplicates = true;

	bAlwaysRelevant = true;
}

template <class T>
T* GetWorldActor(UWorld* World)
{
	TArray<AActor*> DiscoveredActors;
	UGameplayStatics::GetAllActorsOfClass(World, T::StaticClass(), DiscoveredActors);
	if (DiscoveredActors.Num() == 1)
	{
		return Cast<T>(DiscoveredActors[0]);
	}
	return nullptr;
}

void ASpatialTestLoadBalancingData::PrepareTest()
{
	Super::PrepareTest();

	AddStep(TEXT("Create an actor on the main server"), FWorkerDefinition::Server(1), nullptr, [this] {
		ASpatialTestLoadBalancingDataActor* SpawnedActor = GetWorld()->SpawnActor<ASpatialTestLoadBalancingDataActor>();
		check(SpawnedActor->HasAuthority());
		RegisterAutoDestroyActor(SpawnedActor);
		FinishStep();
	});

	AddStep(TEXT("Create an actor on the main server"), FWorkerDefinition::Server(2), nullptr, [this] {
		ASpatialTestLoadBalancingDataOffloadedActor* SpawnedActor = GetWorld()->SpawnActor<ASpatialTestLoadBalancingDataOffloadedActor>();
		check(SpawnedActor->HasAuthority());
		RegisterAutoDestroyActor(SpawnedActor);
		FinishStep();
	});

	constexpr float ActorReceiptTimeout = 5.0f;
	AddStep(
		TEXT("Retrieve the actor on all workers"), FWorkerDefinition::AllWorkers, nullptr, nullptr,
		[this](float) {
			TargetActor = GetWorldActor<ASpatialTestLoadBalancingDataActor>(GetWorld());
			RequireTrue(TargetActor.IsValid(), TEXT("Received main actor"));

			TargetOffloadedActor = GetWorldActor<ASpatialTestLoadBalancingDataOffloadedActor>(GetWorld());
			RequireTrue(TargetOffloadedActor.IsValid(), TEXT("Received offloaded actor"));

			FinishStep();
		},
		ActorReceiptTimeout);

	const float LoadBalancingDataReceiptTimeout = 5.0f;
	AddStep(
		TEXT("Confirm LB group IDs on the server"), FWorkerDefinition::AllServers, nullptr, nullptr,
		[this](float) {
			// ServerWorkers should have interest in LoadBalancingData so it should be available
			TOptional<SpatialGDK::LoadBalancingData> LoadBalancingData = GetLoadBalancingData(TargetActor.Get());
			RequireTrue(LoadBalancingData.IsSet(), TEXT("Main actor entity has LoadBalancingData"));

			TOptional<SpatialGDK::LoadBalancingData> OffloadedLoadBalancingData = GetLoadBalancingData(TargetOffloadedActor.Get());
			RequireTrue(OffloadedLoadBalancingData.IsSet(), TEXT("Offloaded actor entity has LoadBalancingData"));

			const bool bIsValid = LoadBalancingData.IsSet() && OffloadedLoadBalancingData.IsSet()
								  && LoadBalancingData->ActorGroupId != OffloadedLoadBalancingData->ActorGroupId;

			RequireTrue(bIsValid, TEXT("Load balancing group ids are different for the main server and for the offloaded server"));

			FinishStep();
		},
		LoadBalancingDataReceiptTimeout);

	AddStep(TEXT("Put main server actor and offloaded server actor into a single ownership group"), FWorkerDefinition::Server(2), nullptr,
			[this]() {
				AssertTrue(TargetOffloadedActor->HasAuthority(), TEXT("Offloaded actor is owned by the offloaded server"));
				TargetOffloadedActor->SetOwner(TargetActor.Get());
				FinishStep();
			});

	AddStep(
		TEXT("Wait until main and offloaded actors are owned by the main server, and until actor set ID is the same"),
		FWorkerDefinition::AllServers, nullptr, nullptr,
		[this](float) {
			const bool bShouldHaveAuthority = GetLocalWorkerId() == 1;

			RequireTrue(bShouldHaveAuthority == TargetActor->HasAuthority(), TEXT("Main server owns main actor"));

			RequireTrue(bShouldHaveAuthority == TargetOffloadedActor->HasAuthority(), TEXT("Main server owns offloaded actor"));

			RequireTrue(GetLoadBalancingData(TargetActor.Get())->ActorSetId == GetLoadBalancingData(TargetOffloadedActor.Get())->ActorSetId,
						TEXT("Actor set IDs are the same for the main and offloaded actors"));
		},
		LoadBalancingDataReceiptTimeout);
}

TOptional<SpatialGDK::LoadBalancingData> ASpatialTestLoadBalancingData::GetLoadBalancingData(const AActor* Actor) const
{
	USpatialNetDriver* SpatialNetDriver = Cast<USpatialNetDriver>(GetNetDriver());
	const Worker_EntityId ActorEntityId = SpatialNetDriver->PackageMap->GetEntityIdFromObject(Actor);

	if (ensure(ActorEntityId != SpatialConstants::INVALID_ENTITY_ID))
	{
		const SpatialGDK::EntityViewElement& ActorEntity = SpatialNetDriver->Connection->GetView()[ActorEntityId];

		const SpatialGDK::ComponentData* ComponentData =
			ActorEntity.Components.FindByPredicate(SpatialGDK::ComponentIdEquality{ SpatialGDK::LoadBalancingData::ComponentId });

		if (ComponentData != nullptr)
		{
			return SpatialGDK::LoadBalancingData(ComponentData->GetWorkerComponentData());
		}
	}

	return {};
}
