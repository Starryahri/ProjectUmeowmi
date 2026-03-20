// Copyright Epic Games, Inc. All Rights Reserved.

#include "ProjectUmeowmiEditorUtilitiesModule.h"
#include "PUEditorUtilityDialogueExportImport.h"
#include "PUEditorUtilityImportTimeTempModifiers.h"
#include "PUProjectUmeowmiGameInstance.h"
#include "ToolMenus.h"
#include "LevelEditor.h"
#include "Framework/Docking/TabManager.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Misc/PackageName.h"
#include "Engine/DataTable.h"
#include "Engine/Engine.h"
#include "GameMapsSettings.h"

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

		Section.AddMenuEntry(
			"ImportTimeTempModifiers",
			LOCTEXT("ImportTimeTempModifiers", "Import Time/Temp Modifiers from CSV"),
			LOCTEXT("ImportTimeTempModifiersTooltip", "Import time/temperature modifiers from a CSV file into the Ingredient Data Table. Pick the CSV file when prompted."),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateLambda([]()
			{
				IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
				if (!DesktopPlatform) return;

				FString DefaultPath = FPaths::ProjectDir() + TEXT("Ingredient_TimeTemp_Modifiers_Full.csv");
				TArray<FString> OutFiles;
				if (!DesktopPlatform->OpenFileDialog(
					nullptr,
					LOCTEXT("ImportTimeTempModifiersTitle", "Select Time/Temp Modifiers CSV").ToString(),
					FPaths::ProjectDir(),
					TEXT(""),
					TEXT("CSV files (*.csv)|*.csv|All files (*.*)|*.*"),
					EFileDialogFlags::None,
					OutFiles))
				{
					return;
				}
				if (OutFiles.Num() == 0) return;

				UDataTable* IngredientTable = nullptr;
				if (const UGameMapsSettings* Settings = GetDefault<UGameMapsSettings>())
				{
					if (UClass* GIClass = Settings->GameInstanceClass.TryLoadClass<UGameInstance>())
					{
						if (UPUProjectUmeowmiGameInstance* CDO = Cast<UPUProjectUmeowmiGameInstance>(GIClass->GetDefaultObject()))
						{
							IngredientTable = CDO->GetIngredientDataTable();
						}
					}
				}

				if (!IngredientTable)
				{
					UE_LOG(LogTemp, Error, TEXT("ImportTimeTempModifiers: Could not find Ingredient Data Table. Set it in your Game Instance Blueprint (BP_LFCDGameInstance or similar)."));
					return;
				}

				UPUEditorUtilityImportTimeTempModifiers* Util = NewObject<UPUEditorUtilityImportTimeTempModifiers>(GetTransientPackage());
				Util->IngredientDataTable = IngredientTable;
				Util->CSVFilePath = OutFiles[0];
				Util->ImportFromCSV();
			}))
		);
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FProjectUmeowmiEditorUtilitiesModule, ProjectUmeowmiEditorUtilities)
