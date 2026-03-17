// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "EditorUtilityObject.h"
#include "PUEditorUtilityDialogueExportImport.generated.h"

class UDlgDialogue;

/**
 * Editor utility to export and import DlgSystem dialogue text for external editing.
 * Export writes .dlg_human.json files; import applies text changes from those files back to the dialogues.
 *
 * Usage:
 * - Export Selected: Select one or more dialogue assets in the Content Browser, then run Export Selected.
 * - Export All: Exports every dialogue in the project.
 * - Import: Pick a folder containing .dlg_human.json files; only files matching existing dialogues are imported.
 */
UCLASS(Blueprintable)
class PROJECTUMEOWMIEDITORUTILITIES_API UPUEditorUtilityDialogueExportImport : public UEditorUtilityObject
{
	GENERATED_BODY()

public:
	/** Directory for export/import. Relative to project root, or absolute path. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Export/Import")
	FString ExportImportDirectory = TEXT("ExportedDialogue");

	/** Dialogues to export. Populate manually or use Sync From Selection. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue Export/Import")
	TArray<TObjectPtr<UDlgDialogue>> DialoguesToExport;

	/** Sync DialoguesToExport from the currently selected assets in the Content Browser. Call before Export Selected. */
	UFUNCTION(BlueprintCallable, Category = "Dialogue Export/Import")
	void SyncFromContentBrowserSelection();

	/** Export only the dialogues in DialoguesToExport to ExportImportDirectory. */
	UFUNCTION(BlueprintCallable, Category = "Dialogue Export/Import")
	void ExportSelectedDialogues();

	/** Export all dialogues in the project to ExportImportDirectory. */
	UFUNCTION(BlueprintCallable, Category = "Dialogue Export/Import")
	void ExportAllDialogues();

	/** Import from ExportImportDirectory. Imports all .dlg_human.json files found (matching existing dialogues by GUID). */
	UFUNCTION(BlueprintCallable, Category = "Dialogue Export/Import")
	void ImportFromDirectory();

	/** Open a folder picker and set ExportImportDirectory. Returns true if user picked a folder. */
	UFUNCTION(BlueprintCallable, Category = "Dialogue Export/Import")
	bool PickExportImportDirectory();
};
