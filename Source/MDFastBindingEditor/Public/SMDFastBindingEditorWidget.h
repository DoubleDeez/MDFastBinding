#pragma once

#include "WorkflowOrientedApp/WorkflowTabFactory.h"
#include "EditorUndoClient.h"
#include "UObject/WeakFieldPtr.h"

class UMDFastBindingContainer;
class UMDFastBindingInstance;
class FBlueprintEditor;
class FMenuBuilder;
class FObjectPropertyBase;
class IDetailsView;
class SMDFastBindingEditorGraphWidget;
class SMDFastBindingWatchList;

typedef TSet<class UObject*> FGraphPanelSelectionSet;

class SMDFastBindingEditorWidget : public SCompoundWidget, public FSelfRegisteringEditorUndoClient
{
public:

	SLATE_BEGIN_ARGS(SMDFastBindingEditorWidget){}
	SLATE_END_ARGS();

	void Construct(const FArguments&, const TWeakPtr<FBlueprintEditor> InBlueprintEditor);
	void AssignBindingData(UClass* BindingOwnerClass);
	void SelectBinding(UMDFastBindingInstance* InBinding);
	UMDFastBindingContainer* GetSelectedBindingContainer() const;
	UMDFastBindingInstance* GetSelectedBinding() const;
	
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;

	void RefreshGraph() const;

private:
	void OnGraphSelectionChanged(const FGraphPanelSelectionSet& Selection);
	
	EVisibility GetBindingSelectorVisibility() const;
	EVisibility GetBindingTreeVisibility() const;
	
	void PopulateBindingsList();
	TSharedRef<ITableRow> GenerateBindingListWidget(TWeakObjectPtr<UMDFastBindingInstance> Binding, const TSharedRef<STableViewBase>& OwnerTable);
	void SetBindingDisplayName(const FText& InName, ETextCommit::Type CommitType, TWeakObjectPtr<UMDFastBindingInstance> Binding);
	
	FReply OnAddBinding();
	FReply OnDuplicateBinding(TWeakObjectPtr<UMDFastBindingInstance> Binding);
	FReply OnDeleteBinding(TWeakObjectPtr<UMDFastBindingInstance> Binding);

	void OnDetailsPanelPropertyChanged(const FPropertyChangedEvent& Event);

	FText GetBindingValidationTooltip(TWeakObjectPtr<UMDFastBindingInstance> Binding) const;
	const FSlateBrush* GetBindingValidationBrush(TWeakObjectPtr<UMDFastBindingInstance> Binding) const;

	FText GetBindingPerformanceTooltip(TWeakObjectPtr<UMDFastBindingInstance> Binding) const;
	const FSlateBrush* GetBindingPerformanceBrush(TWeakObjectPtr<UMDFastBindingInstance> Binding) const;

	void OnBlueprintCompiled(UBlueprint* Blueprint);

	void UpdateBindingBeingDebugged(UObject* ObjectBeingDebugged, UBlueprint* Blueprint);
	void UpdateBindingBeingDebugged();
	
	FReply OnClearWatches();

	TWeakPtr<FBlueprintEditor> BlueprintEditor;
	TWeakFieldPtr<FObjectPropertyBase> BindingContainerProperty;
	TWeakObjectPtr<UMDFastBindingContainer> BindingContainer;
	TWeakObjectPtr<UMDFastBindingInstance> SelectedBinding;
	TWeakObjectPtr<UMDFastBindingInstance> NewBinding;
	TWeakObjectPtr<UMDFastBindingInstance> BindingBeingDebugged;
	TArray<TWeakObjectPtr<UMDFastBindingInstance>> Bindings;
	TSharedPtr<IDetailsView> DetailsView;
	TSharedPtr<SWidgetSwitcher> DetailSwitcher;
	
	TSharedPtr<SListView<TWeakObjectPtr<UMDFastBindingInstance>>> BindingListView;
	TSharedPtr<SMDFastBindingEditorGraphWidget> BindingGraphWidget;
	TSharedPtr<SMDFastBindingWatchList> WatchList;

	friend class SBindingRow;
};

struct FMDFastBindingEditorSummoner : public FWorkflowTabFactory
{
	FMDFastBindingEditorSummoner(TSharedPtr<FBlueprintEditor> BlueprintEditor);
	
	virtual const FSlateBrush* GetTabIcon(const FWorkflowTabSpawnInfo& Info) const override;
	virtual const FSlateIcon& GetTabSpawnerIcon(const FWorkflowTabSpawnInfo& Info) const override;
	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;

	static const FName TabId;

protected:
	TWeakPtr<FBlueprintEditor> WeakBlueprintEditor;

	FSlateIcon TabIcon;
};
