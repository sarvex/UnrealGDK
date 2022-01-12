// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

#include "EngineClasses/SpatialFastArrayNetSerialize.h"

#include "EngineClasses/SpatialNetBitReader.h"
#include "EngineClasses/SpatialNetBitWriter.h"

namespace SpatialGDK
{
bool FSpatialNetDeltaSerializeInfo::DeltaSerializeRead(USpatialNetDriver* NetDriver, FSpatialNetBitReader& Reader, UObject* Object,
													   int32 ArrayIndex, FProperty* ParentProperty, UScriptStruct* NetDeltaStruct)
{
	FSpatialNetDeltaSerializeInfo NetDeltaInfo;

	SpatialFastArrayNetSerializeCB SerializeCB(NetDriver);

	NetDeltaInfo.Reader = &Reader;
	NetDeltaInfo.Map = Reader.PackageMap;
	NetDeltaInfo.NetSerializeCB = &SerializeCB;
	NetDeltaInfo.Object = Object;

	FStructProperty* ParentStruct = CastField<FStructProperty>(ParentProperty);
	check(ParentStruct);
	void* Destination = ParentStruct->ContainerPtrToValuePtr<void>(Object, ArrayIndex);

	UScriptStruct::ICppStructOps* CppStructOps = NetDeltaStruct->GetCppStructOps();
	check(CppStructOps);

	return CppStructOps->NetDeltaSerialize(NetDeltaInfo, Destination);
}

bool FSpatialNetDeltaSerializeInfo::DeltaSerializeWrite(USpatialNetDriver* NetDriver, FSpatialNetBitWriter& Writer, UObject* Object,
														int32 ArrayIndex, FProperty* ParentProperty, UScriptStruct* NetDeltaStruct)
{
	FSpatialNetDeltaSerializeInfo NetDeltaInfo;

	SpatialFastArrayNetSerializeCB SerializeCB(NetDriver);

	NetDeltaInfo.Writer = &Writer;
	NetDeltaInfo.Map = Writer.PackageMap;
	NetDeltaInfo.NetSerializeCB = &SerializeCB;
	NetDeltaInfo.Object = Object;

	FStructProperty* ParentStruct = CastField<FStructProperty>(ParentProperty);
	check(ParentStruct);
	void* Source = ParentStruct->ContainerPtrToValuePtr<void>(Object, ArrayIndex);

	UScriptStruct::ICppStructOps* CppStructOps = NetDeltaStruct->GetCppStructOps();
	check(CppStructOps);

	return CppStructOps->NetDeltaSerialize(NetDeltaInfo, Source);
}

void SpatialFastArrayNetSerializeCB::NetSerializeStruct(FNetDeltaSerializeInfo& Params)
{
	FBitArchive& Ar = Params.Reader ? static_cast<FBitArchive&>(*Params.Reader) : static_cast<FBitArchive&>(*Params.Writer);
	Params.bOutHasMoreUnmapped = false;
	UScriptStruct* Struct = CastChecked<UScriptStruct>(Params.Struct);

	// Check if struct has custom NetSerialize function, otherwise call standard struct replication
	if (Struct->StructFlags & STRUCT_NetSerializeNative)
	{
		UScriptStruct::ICppStructOps* CppStructOps = Struct->GetCppStructOps();
		check(CppStructOps); // else should not have STRUCT_NetSerializeNative
		bool bSuccess = true;
		if (!CppStructOps->NetSerialize(Ar, Params.Map, bSuccess, reinterpret_cast<uint8*>(Params.Data)))
		{
			Params.bOutHasMoreUnmapped = true;
		}

		// Check the success of the serialization and print a warning if it failed. This is how native handles failed serialization.
		if (!bSuccess)
		{
			UE_LOG(LogSpatialNetSerialize, Warning, TEXT("SpatialFastArrayNetSerialize: NetSerialize %s failed."), *Struct->GetFullName());
		}
	}
	else
	{
		TSharedPtr<FRepLayout> RepLayout = NetDriver->GetStructRepLayout(Struct);

		RepLayout_SerializePropertiesForStruct(*RepLayout, Ar, Params.Map, reinterpret_cast<uint8*>(Params.Data),
											   Params.bOutHasMoreUnmapped);
	}
}

} // namespace SpatialGDK
