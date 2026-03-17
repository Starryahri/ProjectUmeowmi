// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class ProjectUmeowmiEditorUtilities : ModuleRules
{
	public ProjectUmeowmiEditorUtilities(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayTags",
			"ProjectUmeowmi",
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"Blutility",
			"UnrealEd",
			"ImageWrapper",
			"ContentBrowser",
			"DesktopPlatform",
			"DlgSystem",
			"Slate",
			"SlateCore",
			"ToolMenus",
		});

		// ProjectUmeowmi module path for DishCustomization/PUIngredientBase.h
		string ProjectUmeowmiPath = System.IO.Path.GetFullPath(System.IO.Path.Combine(ModuleDirectory, "..", "..", "..", "..", "Source", "ProjectUmeowmi"));
		PublicIncludePaths.Add(ProjectUmeowmiPath);
	}
}
