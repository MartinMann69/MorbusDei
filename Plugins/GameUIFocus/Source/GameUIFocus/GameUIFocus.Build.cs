using UnrealBuildTool;

public class GameUIFocus : ModuleRules
{
	public GameUIFocus(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayTags",
			"InputCore",
			"UMG",
			"Slate",
			"SlateCore"
		});
	}
}
