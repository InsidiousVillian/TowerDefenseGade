using UnrealBuildTool;

public class Towerdefense_GADE : ModuleRules
{
	public Towerdefense_GADE(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine"
		});
	}
}
