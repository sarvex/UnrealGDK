// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "SpatialView/EntityPresenceRecord.h"

namespace SpatialGDK
{
void EntityPresenceRecord::AddEntity(FSpatialEntityId EntityId)
{
	if (EntitiesRemoved.RemoveSingleSwap(EntityId) == 0)
	{
		EntitiesAdded.Add(EntityId);
	}
}

void EntityPresenceRecord::RemoveEntity(FSpatialEntityId EntityId)
{
	if (EntitiesAdded.RemoveSingleSwap(EntityId) == 0)
	{
		EntitiesRemoved.Add(EntityId);
	}
}

void EntityPresenceRecord::Clear()
{
	EntitiesAdded.Empty();
	EntitiesRemoved.Empty();
}

const TArray<FSpatialEntityId>& EntityPresenceRecord::GetEntitiesAdded() const
{
	return EntitiesAdded;
}

const TArray<FSpatialEntityId>& EntityPresenceRecord::GetEntitiesRemoved() const
{
	return EntitiesRemoved;
}

} // namespace SpatialGDK
