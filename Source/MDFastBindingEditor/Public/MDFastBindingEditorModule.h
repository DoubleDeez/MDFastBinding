#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class UBlueprint;
class UMDFastBindingBlueprintCompilerExtension;
class FBlueprintEditor;
class FExtender;
class FLayoutExtender;
class FTabManager;
class FToolBarBuilder;
class FUICommandList;
class FWorkflowAllowedTabSet;

class FMDFastBindingEditorTabBinding : public TSharedFromThis<FMDFastBindingEditorTabBinding>
{
public:

	FMDFastBindingEditorTabBinding();

	~FMDFastBindingEditorTabBinding();
	
	void RegisterBlueprintEditorLayout(FLayoutExtender& Extender);
	void RegisterBlueprintEditorTab(FWorkflowAllowedTabSet& TabFactories, FName InModeName, TSharedPtr<FBlueprintEditor> BlueprintEditor);

private:
	FDelegateHandle BlueprintEditorTabSpawnerHandle;
	FDelegateHandle BlueprintEditorLayoutExtensionHandle;
};

class FMDFastBindingEditorModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
	
	void OpenBindingEditor(TWeakObjectPtr<UObject> EditorObject) const;

private:
	TSharedRef<FExtender> CheckAddBindingEditorToolbarButtons(const TSharedRef<FUICommandList> Commands, const TArray<UObject*> Objects) const;
	void AddBindingEditorToolbarButtons(FToolBarBuilder& ToolBarBuilder, TWeakObjectPtr<UObject> EditorObject) const;
	
	void OnRenameVariable(UBlueprint* Blueprint, UClass* VariableClass, const FName& OldVariableName, const FName& NewVariableName);
	
	TSharedPtr<FMDFastBindingEditorTabBinding> TabBinding;

	FDelegateHandle RenameHandle;
	
	TSharedPtr<class IDesignerExtensionFactory> DesignerExtensionFactory;

	UMDFastBindingBlueprintCompilerExtension* BlueprintCompilerExtension = nullptr;
};
