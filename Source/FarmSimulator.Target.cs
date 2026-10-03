using UnrealBuildTool;
using System.Collections.Generic;

public class FarmSimulatorTarget : TargetRules
{
	public FarmSimulatorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.AddRange(new string[] { "FarmSimulator" });
	}
}
