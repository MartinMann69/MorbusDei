using UnrealBuildTool;

public class MorbusDeiEditor : ModuleRules
{
	public MorbusDeiEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Blutility",
			"Core",
			"CoreUObject",
			"Engine",
			"MorbusDei",
			"UMG"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore",
			"UnrealEd"
		});
	}
}
