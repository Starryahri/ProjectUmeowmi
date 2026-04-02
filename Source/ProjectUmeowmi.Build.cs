// Copyright Epic Games, Inc. All Rights Reserved.

using System.IO;
using UnrealBuildTool;

public class ProjectUmeowmi : ModuleRules
{
	public ProjectUmeowmi(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PrivateIncludePaths.Add(Path.Combine(EngineDirectory, "Plugins/Runtime/CommonUI/Source/CommonUI/Private"));

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"UMG",
			"CommonUI",
			"RadarChart",
			"GameplayTags",
			"Slate",
			"SlateCore",
			"ProceduralMeshComponent",
		"ActorSequence",
		"MovieScene",
		"Niagara"
		});
        PrivateDependencyModuleNames.AddRange(new string[] { "DlgSystem", "RenderCore" });
    }
}
