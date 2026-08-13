// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MorbusDei : ModuleRules
{
	public MorbusDei(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"DeveloperSettings",
			"Engine",
			"EnhancedInput",
			"GameUIFocus",
			"GameplayTags",
			"InputCore",
			"MediaAssets",
			"Niagara",
			"Slate",
			"SlateCore",
			"UMG"
		});
	}
}
