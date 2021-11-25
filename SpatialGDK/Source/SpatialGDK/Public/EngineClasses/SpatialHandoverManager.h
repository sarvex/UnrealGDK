// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#pragma once

#include "Schema/AuthorityIntent.h"
#include "Schema/StandardLibrary.h"
#include "SpatialCommonTypes.h"

#include "SpatialView/EntityComponentTypes.h"

namespace SpatialGDK
{
class FSubView;
class ISpatialOSWorker;

struct LBComponents2
{
	AuthorityIntentV2 Intent;
	AuthorityIntentACK IntentACK;
};

class FSpatialHandoverManager
{
public:
	FSpatialHandoverManager(const FSubView& InActorView, const FSubView& InPartitionView);

	void Advance();

	const TSet<Worker_EntityId_Key>& GetActorsToHandover() { return ActorsToHandover; }
	const TSet<Worker_EntityId_Key>& GetActorsToCheckForAuth() { return ActorsToACK; }

	void Flush(ISpatialOSWorker& Connection, const TSet<Worker_EntityId_Key>& ActorsReleased);

	const TSet<Worker_EntityId_Key>& GetDelegatedPartitions() { return PartitionsDelegated; }
	const TSet<Worker_EntityId_Key>& GetDelegationLost() { return DelegationLost; }

private:
	void PopulateDataStore(const Worker_EntityId EntityId);
	void ApplyComponentUpdate(const Worker_EntityId EntityId, const Worker_ComponentId ComponentId, Schema_ComponentUpdate* Update);
	void ApplyComponentRefresh(const Worker_EntityId EntityId, const Worker_ComponentId ComponentId, Schema_ComponentData* Data);

	void HandleChange(Worker_EntityId EntityId, const LBComponents2& Components);

	const FSubView& ActorView;
	const FSubView& PartitionView;

	TArray<Worker_EntityId> PendingEntityAuthorityChanges;
	TMap<Worker_EntityId_Key, LBComponents2> DataStore;
	TUniqueFunction<void(EntityComponentUpdate)> UpdateSender;

	TSet<Worker_EntityId_Key> PartitionsToACK;
	TSet<Worker_EntityId_Key> OwnedPartitions;
	TSet<Worker_EntityId_Key> ActorsToHandover;
	TSet<Worker_EntityId_Key> ActorsToACK;

	TSet<Worker_EntityId_Key> PartitionsDelegated;
	TSet<Worker_EntityId_Key> DelegationLost;
};

} // namespace SpatialGDK