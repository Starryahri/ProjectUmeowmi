// Copyright Epic Games, Inc. All Rights Reserved.

#include "PUEditorUtilityDialogueExportImport.h"

#if WITH_EDITOR

#include "DlgSystem/DlgDialogue.h"
#include "DlgSystem/DlgManager.h"

#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "DesktopPlatformModule.h"
#include "IDesktopPlatform.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"
#include "GenericPlatform/GenericPlatformFile.h"
#include "Misc/MessageDialog.h"
#include "Modules/ModuleManager.h"
#include "Editor.h"
#include "UnrealEdGlobals.h"

#endif

void UPUEditorUtilityDialogueExportImport::SyncFromContentBrowserSelection()
{
#if WITH_EDITOR
	DialoguesToExport.Empty();

	FContentBrowserModule& ContentBrowserModule = FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	TArray<FAssetData> SelectedAssets;
	ContentBrowserModule.Get().GetSelectedAssets(SelectedAssets);

	for (const FAssetData& AssetData : SelectedAssets)
	{
		if (UDlgDialogue* Dialogue = Cast<UDlgDialogue>(AssetData.GetAsset()))
		{
			DialoguesToExport.Add(Dialogue);
		}
	}

	UE_LOG(LogTemp, Display, TEXT("Dialogue Export/Import: Synced %d dialogue(s) from Content Browser selection."), DialoguesToExport.Num());
#endif
}

static bool RunDialogueCommandlet(const FString& Operation, const FString& OutputDir)
{
	// Ensure DlgSystemEditor module is loaded (commandlet lives there)
	FModuleManager::Get().LoadModule(TEXT("DlgSystemEditor"));

	// Use RunCommandlet console command - runs in-process, no second editor window
	// Use relative path to avoid spaces; commandlet resolves to absolute via ProjectDir
	FString RelDir = OutputDir;
	FPaths::MakePathRelativeTo(RelDir, *FPaths::ProjectDir());
	if (RelDir.IsEmpty() || RelDir == TEXT("."))
	{
		RelDir = TEXT("ExportedDialogue");
	}
	// Quote path if it contains spaces
	FString DirArg = RelDir.Contains(TEXT(" ")) ? FString::Printf(TEXT("\"%s\""), *RelDir) : RelDir;

	FString Cmd = FString::Printf(TEXT("RunCommandlet DlgHumanReadableTextCommandlet -OutputInputDirectory=%s -%s"),
		*DirArg, *Operation);

	UWorld* World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	return GEditor && GEditor->Exec(World, *Cmd);
}

void UPUEditorUtilityDialogueExportImport::ExportSelectedDialogues()
{
#if WITH_EDITOR
	if (DialoguesToExport.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("Dialogue Export/Import: No dialogues selected. Use Sync From Selection or add dialogues to DialoguesToExport."));
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("No dialogues selected. Select dialogue assets in the Content Browser and use Sync From Selection first.")));
		return;
	}

	FString BaseDir = ExportImportDirectory;
	if (FPaths::IsRelative(BaseDir))
	{
		BaseDir = FPaths::Combine(FPaths::ProjectDir(), BaseDir);
	}

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*BaseDir) && !PlatformFile.CreateDirectoryTree(*BaseDir))
	{
		UE_LOG(LogTemp, Error, TEXT("Dialogue Export/Import: Failed to create directory: %s"), *BaseDir);
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(FString::Printf(TEXT("Failed to create directory: %s"), *BaseDir)));
		return;
	}

	// Build set of expected file paths for selected dialogues (e.g. "LuckyFatCatDiner/Dialogue/Props/DLG_Prop_Alter.dlg_human.json")
	TSet<FString> SelectedPaths;
	for (UDlgDialogue* Dlg : DialoguesToExport)
	{
		if (IsValid(Dlg))
		{
			FString Path = Dlg->GetPathName();
			if (Path.RemoveFromStart(TEXT("/Game/")))
			{
				Path = FPaths::SetExtension(Path, TEXT("dlg_human.json"));
				SelectedPaths.Add(Path);
			}
		}
	}

	// Remember files that already existed (from previous exports) - we keep those so Export Selected is additive
	TSet<FString> PreviouslyExistingPaths;
	TArray<FString> PreExistingFiles;
	PlatformFile.FindFilesRecursively(PreExistingFiles, *BaseDir, TEXT(".dlg_human.json"));
	FString BaseDirWithSlash = BaseDir;
	if (!BaseDirWithSlash.EndsWith(TEXT("/")) && !BaseDirWithSlash.EndsWith(TEXT("\\")))
	{
		BaseDirWithSlash += TEXT("/");
	}
	for (const FString& FilePath : PreExistingFiles)
	{
		FString RelPath = FilePath;
		if (FPaths::MakePathRelativeTo(RelPath, *BaseDirWithSlash))
		{
			RelPath.ReplaceInline(TEXT("\\"), TEXT("/"));
			PreviouslyExistingPaths.Add(RelPath);
		}
	}

	RunDialogueCommandlet(TEXT("Export"), BaseDir);

	// Remove only files that are neither in our selection nor were previously exported (additive: keep existing + newly selected)
	if (SelectedPaths.Num() > 0)
	{
		TArray<FString> FoundFiles;
		PlatformFile.FindFilesRecursively(FoundFiles, *BaseDir, TEXT(".dlg_human.json"));
		int32 Removed = 0;
		for (const FString& FilePath : FoundFiles)
		{
			FString RelPath = FilePath;
			if (FPaths::MakePathRelativeTo(RelPath, *BaseDirWithSlash))
			{
				RelPath.ReplaceInline(TEXT("\\"), TEXT("/"));
			}
			else
			{
				RelPath = FilePath;
				if (RelPath.StartsWith(BaseDir))
				{
					RelPath.RightChopInline(BaseDir.Len());
					while (RelPath.StartsWith(TEXT("/")) || RelPath.StartsWith(TEXT("\\")))
					{
						RelPath = RelPath.Mid(1);
					}
				}
				RelPath.ReplaceInline(TEXT("\\"), TEXT("/"));
			}
			// Keep if: in current selection (just updated) OR was in folder before this export (from earlier session)
			const bool bKeep = SelectedPaths.Contains(RelPath) || PreviouslyExistingPaths.Contains(RelPath);
			if (!bKeep)
			{
				PlatformFile.DeleteFile(*FilePath);
				Removed++;
			}
		}
		if (Removed > 0)
		{
			UE_LOG(LogTemp, Display, TEXT("Dialogue Export/Import: Removed %d non-selected dialogue file(s) (kept previously exported)."), Removed);
		}
	}

	UE_LOG(LogTemp, Display, TEXT("Dialogue Export/Import: Exported %d selected dialogue(s)."), DialoguesToExport.Num());
	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(FString::Printf(TEXT("Exported %d dialogue(s) to:\n%s"), DialoguesToExport.Num(), *BaseDir)));
#endif
}

void UPUEditorUtilityDialogueExportImport::ExportAllDialogues()
{
#if WITH_EDITOR
	FString BaseDir = ExportImportDirectory;
	if (FPaths::IsRelative(BaseDir))
	{
		BaseDir = FPaths::Combine(FPaths::ProjectDir(), BaseDir);
	}

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*BaseDir) && !PlatformFile.CreateDirectoryTree(*BaseDir))
	{
		UE_LOG(LogTemp, Error, TEXT("Dialogue Export/Import: Failed to create directory: %s"), *BaseDir);
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(FString::Printf(TEXT("Failed to create directory: %s"), *BaseDir)));
		return;
	}

	RunDialogueCommandlet(TEXT("Export"), BaseDir);
	UE_LOG(LogTemp, Display, TEXT("Dialogue Export/Import: Export complete. See Output Log for details."));
	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("Dialogue export complete. Check Output Log for details.")));
#endif
}

void UPUEditorUtilityDialogueExportImport::ImportFromDirectory()
{
#if WITH_EDITOR
	FString BaseDir = ExportImportDirectory;
	if (FPaths::IsRelative(BaseDir))
	{
		BaseDir = FPaths::Combine(FPaths::ProjectDir(), BaseDir);
	}

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (!PlatformFile.DirectoryExists(*BaseDir))
	{
		UE_LOG(LogTemp, Error, TEXT("Dialogue Export/Import: Directory does not exist: %s"), *BaseDir);
		FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(FString::Printf(TEXT("Directory does not exist: %s"), *BaseDir)));
		return;
	}

	RunDialogueCommandlet(TEXT("Import"), BaseDir);
	UE_LOG(LogTemp, Display, TEXT("Dialogue Export/Import: Import complete. See Output Log for details."));
	FMessageDialog::Open(EAppMsgType::Ok, FText::FromString(TEXT("Dialogue import complete. Check Output Log for details. Save your assets (Ctrl+S) to persist changes.")));
#endif
}

bool UPUEditorUtilityDialogueExportImport::PickExportImportDirectory()
{
#if WITH_EDITOR
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	if (!DesktopPlatform)
	{
		return false;
	}

	FString CurrentPath = ExportImportDirectory;
	if (FPaths::IsRelative(CurrentPath))
	{
		CurrentPath = FPaths::Combine(FPaths::ProjectDir(), CurrentPath);
	}

	FString OutFolder;
	if (DesktopPlatform->OpenDirectoryDialog(
		nullptr,
		NSLOCTEXT("ProjectUmeowmi", "PickDialogueFolder", "Select Dialogue Export/Import Folder").ToString(),
		CurrentPath,
		OutFolder) && !OutFolder.IsEmpty())
	{
		FString ProjectDir = FPaths::ProjectDir();
		if (OutFolder.StartsWith(ProjectDir))
		{
			OutFolder.RightChopInline(ProjectDir.Len());
			while (OutFolder.StartsWith(TEXT("/")))
			{
				OutFolder = OutFolder.Mid(1);
			}
		}
		ExportImportDirectory = OutFolder.IsEmpty() ? TEXT(".") : OutFolder;
		return true;
	}
#endif
	return false;
}
