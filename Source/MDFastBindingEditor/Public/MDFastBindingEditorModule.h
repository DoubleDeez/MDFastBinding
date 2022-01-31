#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FTabManager;
class FBlueprintEditor;
class FWorkflowAllowedTabSet;
class FLayoutExtender;

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
	
	static bool DoesObjectHaveFastBindings(const UObject& Object);
	static bool DoesClassHaveFastBindings(const UStruct* Class);

private:
	TSharedRef<FExtender> CheckAddBindingEditorToolbarButton(const TSharedRef<FUICommandList> Commands, const TArray<UObject*> Objects) const;
	void AddBindingEditorToolbarButton(FToolBarBuilder& ToolBarBuilder, TWeakObjectPtr<UObject> EditorObject) const;
	
	TSharedPtr<FMDFastBindingEditorTabBinding> TabBinding;
};
