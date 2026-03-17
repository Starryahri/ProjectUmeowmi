// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectUmeowmiEditorUtilitiesModule.h"
#include "PUEditorUtilityDialogueExportImport.h"
#include "ToolMenus.h"
#include "LevelEditor.h"
#include "Framework/Docking/TabManager.h"

#define LOCTEXT_NAMESPACE "FProjectUmeowmiEditorUtilitiesModule"

void FProjectUmeowmiEditorUtilitiesModule::StartupModule()
{
	RegisterMenus();
}

void FProjectUmeowmiEditorUtilitiesModule::ShutdownModule()
{
}

void FProjectUmeowmiEditorUtilitiesModule::RegisterMenus()
{
	FToolMenuOwnerScoped OwnerScoped(this);
	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
	if (Menu)
	{
		FToolMenuSection& Section = Menu->FindOrAddSection("ProjectUmeowmi");
		Section.Label = LOCTEXT("ProjectUmeowmiSection", "Project Umeowmi");

		Section.AddMenuEntry(
			"ExportSelectedDialogues",
			LOCTEXT("ExportSelectedDialogues", "Export Selected Dialogues"),
			LOCTEXT("ExportSelectedDialoguesTooltip", "Export selected dialogue assets to ExportedDialogue folder. Select dialogues in Content Browser first."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([]()
			{
				UPUEditorUtilityDialogueExportImport* Util = NewObject<UPUEditorUtilityDialogueExportImport>(GetTransientPackage());
				Util->ExportImportDirectory = TEXT("ExportedDialogue");
				Util->SyncFromContentBrowserSelection();
				Util->ExportSelectedDialogues();
			}))
		);

		Section.AddMenuEntry(
			"ExportAllDialogues",
			LOCTEXT("ExportAllDialogues", "Export All Dialogues"),
			LOCTEXT("ExportAllDialoguesTooltip", "Export all dialogue assets in the project to ExportedDialogue folder."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([]()
			{
				UPUEditorUtilityDialogueExportImport* Util = NewObject<UPUEditorUtilityDialogueExportImport>(GetTransientPackage());
				Util->ExportImportDirectory = TEXT("ExportedDialogue");
				Util->ExportAllDialogues();
			}))
		);

		Section.AddMenuEntry(
			"ImportDialogues",
			LOCTEXT("ImportDialogues", "Import Dialogues"),
			LOCTEXT("ImportDialoguesTooltip", "Import dialogue text from ExportedDialogue folder. Edit .dlg_human.json files externally first."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([]()
			{
				UPUEditorUtilityDialogueExportImport* Util = NewObject<UPUEditorUtilityDialogueExportImport>(GetTransientPackage());
				Util->ExportImportDirectory = TEXT("ExportedDialogue");
				Util->ImportFromDirectory();
			}))
		);
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FProjectUmeowmiEditorUtilitiesModule, ProjectUmeowmiEditorUtilities)
