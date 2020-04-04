// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class Waypoints : ModuleRules
{
    public Waypoints(ReadOnlyTargetRules Target) : base(Target)
    {
        bEnforceIWYU = true;
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "NavigationSystem",
                "AIModule",
                "GameplayTasks",
			}
        );


        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
			}
        );

        if (Target.Type == TargetType.Editor)
        {
            PrivateDependencyModuleNames.AddRange(
                new string[]
                {
                    "UnrealEd"
                }
            );
        }
    }
}
