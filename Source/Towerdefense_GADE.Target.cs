using UnrealBuildTool;

public class Towerdefense_GADETarget : TargetRules
{
	public Towerdefense_GADETarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		ExtraModuleNames.Add("Towerdefense_GADE");
	}
}
