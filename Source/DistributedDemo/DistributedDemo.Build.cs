// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class DistributedDemo : ModuleRules
{
	public DistributedDemo(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"StateTreeModule",
			"GameplayStateTreeModule",
			"UMG",
			"Slate",
            "GameplayAbilities",
            "GameplayTags",
            "GameplayTasks",
			"DedicatedServers"
        });

        if (Target.Type == TargetType.Server)
        {
            PublicDependencyModuleNames.Add("GameLiftServerSDK");
        }
        else
        {
            PublicDefinitions.Add("WITH_GAMELIFT=0");
        }
        bEnableExceptions = true;

        PrivateDependencyModuleNames.AddRange(new string[] { });

		PublicIncludePaths.AddRange(new string[] {
			"DistributedDemo",
			"DistributedDemo/Variant_Platforming",
			"DistributedDemo/Variant_Platforming/Animation",
			"DistributedDemo/Variant_Combat",
			"DistributedDemo/Variant_Combat/AI",
			"DistributedDemo/Variant_Combat/Animation",
			"DistributedDemo/Variant_Combat/Gameplay",
			"DistributedDemo/Variant_Combat/Interfaces",
			"DistributedDemo/Variant_Combat/UI",
			"DistributedDemo/Variant_SideScrolling",
			"DistributedDemo/Variant_SideScrolling/AI",
			"DistributedDemo/Variant_SideScrolling/Gameplay",
			"DistributedDemo/Variant_SideScrolling/Interfaces",
			"DistributedDemo/Variant_SideScrolling/UI"
		});

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
