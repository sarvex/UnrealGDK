// Copyright (c) Improbable Worlds Ltd, All Rights Reserved

using System.IO;
using UnrealBuildTool;

public class SpatialGDKFunctionalTests : ModuleRules
{
    public SpatialGDKFunctionalTests(ReadOnlyTargetRules Target) : base(Target)
    {
        bLegacyPublicIncludePaths = false;
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        bUseUnity = false;

        PublicIncludePaths.Add(ModuleDirectory);
        
        PrivateDependencyModuleNames.AddRange(
            new string[] {
                "SpatialGDK",
                "SpatialGDKServices",
                "Core",
                "CoreUObject",
                "Engine",
                "FunctionalTesting",
                "HTTP",
                "UnrealEd"
            });
    }
}
