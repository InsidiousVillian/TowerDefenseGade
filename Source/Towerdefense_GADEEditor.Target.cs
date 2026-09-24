using UnrealBuildTool;

public class Towerdefense_GADEEditorTarget : TargetRules
{
	public Towerdefense_GADEEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V6;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		ExtraModuleNames.Add("Towerdefense_GADE");
	}
}
