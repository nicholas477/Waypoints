// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class WaypointsEditorExtension : ModuleRules
{
    public WaypointsEditorExtension(ReadOnlyTargetRules Target) : base(Target)
    {
        bEnforceIWYU = true;
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        CppStandard = CppStandardVersion.Cpp17;

        PrivateIncludePaths.AddRange(new string[] { "WaypointsEditorExtension/Private" });
        PublicIncludePaths.AddRange(new string[] { "WaypointsEditorExtension/Public" });

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
			}
        );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "UnrealEd",
                "Slate",
                "SlateCore",
                "LevelEditor",
                "Waypoints"
			}
        );
    }
}
