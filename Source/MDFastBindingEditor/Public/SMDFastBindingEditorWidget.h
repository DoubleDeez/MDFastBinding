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
class SMDFastBindingInstanceRow;
class SMDFastBindingEditorGraphWidget;
class SMDFastBindingWatchList;

typedef TSet<class UObject*> FGraphPanelSelectionSet;

class SMDFastBindingEditorWidget : public SCompoundWidget, public FSelfRegisteringEditorUndoClient
{
public:

	SLATE_BEGIN_ARGS(SMDFastBindingEditorWidget){}
	SLATE_END_ARGS();

	void Construct(const FArguments&, const TWeakPtr<FBlueprintEditor> InBlueprintEditor);
	void AssignBindingData(UBlueprint* BindingOwnerBP);
	void SelectBinding(UMDFastBindingInstance* InBinding);
	UMDFastBindingContainer* GetSelectedBindingContainer() const;
	UMDFastBindingInstance* GetSelectedBinding() const;
	
	UMDFastBindingInstance* GetNewBinding() const;
	void ResetNewBinding();
	
	FReply OnAddBinding();
	FReply OnDuplicateBinding(TWeakObjectPtr<UMDFastBindingInstance> Binding);
	FReply OnDeleteBinding(TWeakObjectPtr<UMDFastBindingInstance> Binding);
	
	virtual void PostUndo(bool bSuccess) override;
	virtual void PostRedo(bool bSuccess) override;

	void RefreshGraph() const;

private:
	void OnGraphSelectionChanged(const FGraphPanelSelectionSet& Selection);
	
	EVisibility GetBindingSelectorVisibility() const;
	EVisibility GetBindingTreeVisibility() const;
	
	void PopulateBindingsList();
	TSharedRef<ITableRow> GenerateBindingListRowWidget(TWeakObjectPtr<UMDFastBindingInstance> Binding, const TSharedRef<STableViewBase>& OwnerTable);

	TOptional<EItemDropZone> OnCanAcceptDropBinding(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TWeakObjectPtr<UMDFastBindingInstance> TargetBindingPtr, TSharedRef<SMDFastBindingInstanceRow> BindingRow) const;
	FReply OnAcceptDropBinding(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TWeakObjectPtr<UMDFastBindingInstance> TargetBindingPtr, TSharedRef<SMDFastBindingInstanceRow> BindingRow);

	void OnDetailsPanelPropertyChanged(const FPropertyChangedEvent& Event);

	void OnBlueprintCompiled(UBlueprint* Blueprint);

	void UpdateBindingBeingDebugged(UObject* ObjectBeingDebugged);
	void UpdateBindingBeingDebugged();
	
	FReply OnClearWatches();

	TWeakPtr<FBlueprintEditor> BlueprintEditor;
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
