using UnrealBuildTool;
using System.Collections.Generic;

public class FarmSimulatorEditorTarget : TargetRules
{
	public FarmSimulatorEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.AddRange(new string[] { "FarmSimulator" });
	}
}
