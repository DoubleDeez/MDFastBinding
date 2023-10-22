#include "SMDFastBindingEditorWidget.h"
#include "BindingDestinations/MDFastBindingDestinationBase.h"
#include "BlueprintExtension/MDFastBindingWidgetBlueprintExtension.h"
#include "BlueprintEditor.h"
#include "ClassViewerFilter.h"
#include "ClassViewerModule.h"
#include "Debug/MDFastBindingEditorDebug.h"
#include "Debug/MDFastBindingDebugPersistentData.h"
#include "Kismet2/SClassPickerDialog.h"
#include "MDFastBindingContainer.h"
#include "MDFastBindingEditorModule.h"
#include "MDFastBindingEditorStyle.h"
#include "MDFastBindingHelpers.h"
#include "Graph/MDFastBindingGraphNode.h"
#include "MDFastBindingInstance.h"
#include "PropertyEditorModule.h"
#include "Misc/MessageDialog.h"
#include "ScopedTransaction.h"
#include "Graph/SMDFastBindingEditorGraphWidget.h"
#include "SMDFastBindingInstanceRow.h"
#include "WidgetBlueprint.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Util/MDFastBindingEditorHelpers.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"

#define LOCTEXT_NAMESPACE "MDFastBindingEditorWidget"

namespace SMDFastBindingEditorWidget_Private
{
	const int32 BindingListIndex = 0;
	const int32 NodeDetailsIndex = 1;
}


class FMDFastBindingDestinationClassFilter : public IClassViewerFilter
{
	const UClass* RootClass = UMDFastBindingDestinationBase::StaticClass();
	const EClassFlags DisallowedClassFlags = CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists;

	virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef<FClassViewerFilterFuncs> InFilterFuncs ) override
	{
		return !InClass->HasAnyClassFlags(DisallowedClassFlags) && InClass->IsChildOf(RootClass);
	}

	virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef<const IUnloadedBlueprintData> InUnloadedClassData, TSharedRef<FClassViewerFilterFuncs> InFilterFuncs) override
	{
		return !InUnloadedClassData->HasAnyClassFlags(DisallowedClassFlags) && InUnloadedClassData->IsChildOf(RootClass);
	}
};

void SMDFastBindingEditorWidget::Construct(const FArguments&, const TWeakPtr<FBlueprintEditor> InBlueprintEditor)
{
	BlueprintEditor = InBlueprintEditor;

	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.bShowPropertyMatrixButton = false;
	DetailsViewArgs.bHideSelectionTip = true;
	DetailsView = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateDetailView(DetailsViewArgs);
	DetailsView->OnFinishedChangingProperties().AddSP(this, &SMDFastBindingEditorWidget::OnDetailsPanelPropertyChanged);

	UBlueprint* Blueprint = InBlueprintEditor.Pin()->GetBlueprintObj();
	AssignBindingData(Blueprint);
	Blueprint->OnCompiled().AddSP(this, &SMDFastBindingEditorWidget::OnBlueprintCompiled);
	Blueprint->OnSetObjectBeingDebugged().AddSP(this, &SMDFastBindingEditorWidget::UpdateBindingBeingDebugged);


	BindingListView = SNew(SListView<TWeakObjectPtr<UMDFastBindingInstance>>)
		.ListItemsSource(&Bindings)
		.SelectionMode(ESelectionMode::None)
		.OnGenerateRow(this, &SMDFastBindingEditorWidget::GenerateBindingListRowWidget);

	BindingListView->SetSelection(GetSelectedBinding());

	BindingGraphWidget = SNew(SMDFastBindingEditorGraphWidget, Blueprint)
		.OnSelectionChanged(this, &SMDFastBindingEditorWidget::OnGraphSelectionChanged);
	BindingGraphWidget->SetBinding(GetSelectedBinding());

	UpdateBindingBeingDebugged(Blueprint->GetObjectBeingDebugged());

	WatchList = SNew(SMDFastBindingWatchList);
	WatchList->SetReferences(GetSelectedBinding(), BindingBeingDebugged.Get());

	UMDFastBindingDebugPersistentData::Get().OnWatchListChanged.AddSP(WatchList.Get(), &SMDFastBindingWatchList::RefreshList);

	ChildSlot
	[
		SNew(SSplitter)
		+SSplitter::Slot()
		.MinSize(350.f)
		.Value(0.15f)
		[
			SNew(SSplitter)
			.Orientation(Orient_Vertical)
			+SSplitter::Slot()
			.Value(0.5f)
			[
				SAssignNew(DetailSwitcher, SWidgetSwitcher)
				+SWidgetSwitcher::Slot()
				.Padding(4.f)
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Top)
					.AutoHeight()
					[
						SNew(STextBlock).Text(LOCTEXT("BindingListLabel", "Bindings:"))
					]
					+SVerticalBox::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					.FillHeight(1.f)
					.Padding(0.f, 4.f)
					[
						BindingListView.ToSharedRef()
					]
					+SVerticalBox::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Top)
					.AutoHeight()
					[
						SNew(SHorizontalBox)
						.Visibility(this, &SMDFastBindingEditorWidget::GetSuperClassBindingsVisibility)
						+SHorizontalBox::Slot()
						.HAlign(HAlign_Left)
						.VAlign(VAlign_Center)
						.AutoWidth()
						.Padding(0, 2.f, 4.f, 4.f)
						[
							SNew(SImage)
							.Image(FAppStyle::Get().GetBrush(TEXT("Icons.Info")))
						]
						+SHorizontalBox::Slot()
						.HAlign(HAlign_Left)
						.VAlign(VAlign_Center)
						.AutoWidth()
						.Padding(0, 2.f, 0, 4.f)
						[
							SNew(STextBlock).Text(LOCTEXT("SuperClassHasBindingsWarning", "A Blueprint this Blueprint extends from has bindings."))
						]
					]
					+SVerticalBox::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Top)
					.AutoHeight()
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.OnClicked(this, &SMDFastBindingEditorWidget::OnAddBinding)
						[
							SNew(SHorizontalBox)
							+SHorizontalBox::Slot()
							.HAlign(HAlign_Left)
							.VAlign(VAlign_Center)
							.AutoWidth()
							.Padding(0, 0, 4.f, 0)
							[
								SNew(SImage)
								.Image(FAppStyle::Get().GetBrush(TEXT("EditableComboBox.Add")))
							]
							+SHorizontalBox::Slot()
							.HAlign(HAlign_Left)
							.VAlign(VAlign_Center)
							.AutoWidth()
							[
								SNew(STextBlock).Text(LOCTEXT("AddBindingButtonLabel", "Add Binding"))
							]
						]
					]
				]
				+SWidgetSwitcher::Slot()
				[
					DetailsView.ToSharedRef()
				]
			]
			+SSplitter::Slot()
			.Value(0.5f)
			[
				SNew(SVerticalBox)
				+SVerticalBox::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Top)
				.AutoHeight()
				.Padding(4.f)
				[
					SNew(STextBlock).Text(LOCTEXT("WatchListLabel", "Watched Pins:"))
				]
				+SVerticalBox::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Fill)
				.FillHeight(1.f)
				[
					WatchList.ToSharedRef()
				]
				+SVerticalBox::Slot()
				.HAlign(HAlign_Fill)
				.VAlign(VAlign_Top)
				.AutoHeight()
				.Padding(4.f)
				[
					SNew(SButton)
					.HAlign(HAlign_Center)
					.OnClicked(this, &SMDFastBindingEditorWidget::OnClearWatches)
					[
						SNew(SHorizontalBox)
						+SHorizontalBox::Slot()
						.HAlign(HAlign_Left)
						.VAlign(VAlign_Center)
						.AutoWidth()
						.Padding(0, 0, 4.f, 0)
						[
							SNew(SImage)
							.Image(FAppStyle::Get().GetBrush(TEXT("GenericCommands.Delete")))
						]
						+SHorizontalBox::Slot()
						.HAlign(HAlign_Left)
						.VAlign(VAlign_Center)
						.AutoWidth()
						[
							SNew(STextBlock).Text(LOCTEXT("ClearWatchesButtonLabel", "Clear Watches"))
						]
					]
				]
			]
		]
		+SSplitter::Slot()
		[
			BindingGraphWidget.ToSharedRef()
		]
	];
}
void SMDFastBindingEditorWidget::RefreshGraph() const
{
	if (BindingGraphWidget.IsValid())
	{
		BindingGraphWidget->RefreshGraph();
	}
}

void SMDFastBindingEditorWidget::AssignBindingData(UBlueprint* BindingOwnerBP)
{
	BindingContainer.Reset();

	if (BindingOwnerBP != nullptr)
	{
		BindingContainer = MDFastBindingEditorHelpers::FindBindingContainerCDOInBlueprint(BindingOwnerBP);
		if (BindingContainer.IsValid())
		{
			// If this is a widget's binding container and it didn't come from an extension then we should upgrade the container to be extension based
			if (const UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(BindingOwnerBP))
			{
				const UMDFastBindingWidgetBlueprintExtension* BPExtension = UMDFastBindingWidgetBlueprintExtension::GetExtension<UMDFastBindingWidgetBlueprintExtension>(WidgetBP);
				if (BPExtension == nullptr || BPExtension->GetBindingContainer() != BindingContainer)
				{
					MDFastBindingEditorHelpers::InitBindingContainerInBlueprint(BindingOwnerBP, BindingContainer.Get());
					MDFastBindingEditorHelpers::ClearBindingContainerCDOInClass(BindingOwnerBP->GeneratedClass);
				}
			}
		}
		else
		{
			MDFastBindingEditorHelpers::InitBindingContainerInBlueprint(BindingOwnerBP);
		}

		if (const UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(BindingOwnerBP))
		{
			UMDFastBindingWidgetBlueprintExtension* BPExtension = UMDFastBindingWidgetBlueprintExtension::GetExtension<UMDFastBindingWidgetBlueprintExtension>(WidgetBP);
			if (IsValid(BPExtension))
			{
				BPExtension->OnBindingsUpdatedExternally.AddSP(this, &SMDFastBindingEditorWidget::OnBindingsExternallyModified);
			}
		}
	}

	PopulateBindingsList();
}

void SMDFastBindingEditorWidget::SelectBinding(UMDFastBindingInstance* InBinding)
{
	if (!Bindings.Contains(InBinding))
	{
		return;
	}

	SelectedBinding = InBinding;
	if (BindingGraphWidget.IsValid())
	{
		BindingGraphWidget->SetBinding(GetSelectedBinding());
	}

	if (BindingListView.IsValid())
	{
		BindingListView->SetSelection(InBinding);
		BindingListView->RequestScrollIntoView(SelectedBinding);
	}

	UpdateBindingBeingDebugged();
}

UMDFastBindingContainer* SMDFastBindingEditorWidget::GetSelectedBindingContainer() const
{
	return BindingContainer.Get();
}

UMDFastBindingInstance* SMDFastBindingEditorWidget::GetSelectedBinding() const
{
	if (Bindings.Contains(SelectedBinding) && GetSelectedBindingContainer() != nullptr)
	{
		return SelectedBinding.Get();
	}

	return nullptr;
}

UMDFastBindingInstance* SMDFastBindingEditorWidget::GetNewBinding() const
{
	return NewBinding.Get();
}

void SMDFastBindingEditorWidget::ResetNewBinding()
{
	NewBinding.Reset();
}

void SMDFastBindingEditorWidget::PostUndo(bool bSuccess)
{
	PopulateBindingsList();
	BindingListView->SetSelection(SelectedBinding);
	BindingGraphWidget->SetBinding(GetSelectedBinding());
}

void SMDFastBindingEditorWidget::PostRedo(bool bSuccess)
{
	PopulateBindingsList();
	BindingListView->SetSelection(SelectedBinding);
	BindingGraphWidget->SetBinding(GetSelectedBinding());
}

void SMDFastBindingEditorWidget::OnGraphSelectionChanged(const FGraphPanelSelectionSet& Selection)
{
	if (DetailSwitcher.IsValid())
	{
		if (Selection.Num() == 0)
		{
			DetailSwitcher->SetActiveWidgetIndex(SMDFastBindingEditorWidget_Private::BindingListIndex);
			if (DetailsView.IsValid())
			{
				DetailsView->SetObject(nullptr);
			}
		}
		else if (DetailsView.IsValid())
		{
			TArray<UObject*> BindingObjects;
			for (UObject* SelectedObject : Selection)
			{
				if (const UMDFastBindingGraphNode* GraphNode = Cast<UMDFastBindingGraphNode>(SelectedObject))
				{
					if (UMDFastBindingObject* GraphObject = GraphNode->GetBindingObject())
					{
						BindingObjects.Add(GraphObject);
					}
				}
			}
			DetailsView->SetObjects(BindingObjects);
			DetailSwitcher->SetActiveWidgetIndex(SMDFastBindingEditorWidget_Private::NodeDetailsIndex);
		}
	}
}

void SMDFastBindingEditorWidget::OnBindingsExternallyModified()
{
	PopulateBindingsList();
}

EVisibility SMDFastBindingEditorWidget::GetBindingSelectorVisibility() const
{
	return (GetSelectedBindingContainer() != nullptr) ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SMDFastBindingEditorWidget::GetBindingTreeVisibility() const
{
	return (GetSelectedBindingContainer() != nullptr && SelectedBinding.IsValid()) ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SMDFastBindingEditorWidget::GetSuperClassBindingsVisibility() const
{
	const UBlueprint* Blueprint = GetBlueprint();
	const bool bDoesClassHaveSuperClassBindings = Blueprint != nullptr
		&& FMDFastBindingHelpers::DoesClassHaveSuperClassBindings(Cast<UWidgetBlueprintGeneratedClass>(Blueprint->GeneratedClass));

	return bDoesClassHaveSuperClassBindings ? EVisibility::Visible : EVisibility::Collapsed;
}

void SMDFastBindingEditorWidget::PopulateBindingsList()
{
	if (const UMDFastBindingContainer* Container = GetSelectedBindingContainer())
	{
		Bindings = TArray<TWeakObjectPtr<UMDFastBindingInstance>>(Container->GetBindings());
	}
	else
	{
		Bindings.Empty();
	}

	if (BindingListView.IsValid())
	{
		BindingListView->RequestListRefresh();
	}

	if (!SelectedBinding.IsValid() || !Bindings.Contains(SelectedBinding))
	{
		if (Bindings.Num() > 0)
		{
			SelectBinding(Bindings[0].Get());
		}
		else
		{
			SelectBinding(nullptr);
		}
	}
}

TSharedRef<ITableRow> SMDFastBindingEditorWidget::GenerateBindingListRowWidget(TWeakObjectPtr<UMDFastBindingInstance> Binding, const TSharedRef<STableViewBase>& OwnerTable)
{
	TSharedRef<SMDFastBindingInstanceRow> BindingRow = SNew(SMDFastBindingInstanceRow, SharedThis(this), Binding);
	return SNew(STableRow<TWeakObjectPtr<UMDFastBindingInstance>>, OwnerTable)
		.OnCanAcceptDrop(this, &SMDFastBindingEditorWidget::OnCanAcceptDropBinding, BindingRow)
		.OnAcceptDrop(this, &SMDFastBindingEditorWidget::OnAcceptDropBinding, BindingRow)
		.OnDragLeave_Lambda([](const FDragDropEvent& DragDropEvent)
		{
			if (const TSharedPtr<FDecoratedDragDropOp> DecoratedOp = DragDropEvent.GetOperationAs<FDecoratedDragDropOp>())
			{
				DecoratedOp->ResetToDefaultToolTip();
			}
		})
		[
			BindingRow
		];
}

TOptional<EItemDropZone> SMDFastBindingEditorWidget::OnCanAcceptDropBinding(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TWeakObjectPtr<UMDFastBindingInstance> TargetBindingPtr, TSharedRef<SMDFastBindingInstanceRow> BindingRow) const
{
	if (const TSharedPtr<FMDFastBindingInstanceRowDragDropOp> BindingDropOp = DragDropEvent.GetOperationAs<FMDFastBindingInstanceRowDragDropOp>())
	{
		if (BindingDropOp->IsValidTarget(TargetBindingPtr, DragDropEvent.GetScreenSpacePosition(), BindingRow->GetTickSpaceGeometry()))
		{
			BindingDropOp->SetValidTarget(true);
			return FMDFastBindingInstanceRowDragDropOp::CalculateDropZoneRelativeToGeometry(DragDropEvent.GetScreenSpacePosition(), BindingRow->GetTickSpaceGeometry());
		}

		BindingDropOp->SetValidTarget(false);
	}

	return TOptional<EItemDropZone>();
}

FReply SMDFastBindingEditorWidget::OnAcceptDropBinding(const FDragDropEvent& DragDropEvent, EItemDropZone DropZone, TWeakObjectPtr<UMDFastBindingInstance> TargetBindingPtr, TSharedRef<SMDFastBindingInstanceRow> BindingRow)
{
	const TSharedPtr<FMDFastBindingInstanceRowDragDropOp> BindingDropOp = DragDropEvent.GetOperationAs<FMDFastBindingInstanceRowDragDropOp>();
	if (!BindingDropOp.IsValid())
	{
		return FReply::Unhandled();
	}

	UMDFastBindingInstance* DraggedBinding = BindingDropOp->CachedBindingPtr.Get();
	const UMDFastBindingInstance* TargetBinding = TargetBindingPtr.Get();
	if (DraggedBinding == nullptr || TargetBinding == nullptr)
	{
		return FReply::Unhandled();
	}

	const FGeometry& BindingRowGeometry = BindingRow->GetTickSpaceGeometry();
	if (!BindingDropOp->IsValidTarget(TargetBindingPtr, DragDropEvent.GetScreenSpacePosition(), BindingRowGeometry))
	{
		return FReply::Unhandled();
	}

	const int32 DropIndex = FMDFastBindingInstanceRowDragDropOp::CalculateDropIndex(DragDropEvent.GetScreenSpacePosition(), BindingRowGeometry, TargetBinding->GetBindingIndex());
	if (DropIndex == INDEX_NONE)
	{
		return FReply::Unhandled();
	}

	{
		FScopedTransaction Transaction(LOCTEXT("ReorderBindings", "Reorder Bindings"));
		DraggedBinding->MoveToIndex(DropIndex);
	}

	PopulateBindingsList();

	return FReply::Handled();
}

FReply SMDFastBindingEditorWidget::OnAddBinding()
{
	if (UMDFastBindingContainer* Container = GetSelectedBindingContainer())
	{
		FScopedTransaction Transaction = FScopedTransaction(LOCTEXT("AddBindingTransaction", "Added Binding"));
		Container->Modify();

		if (UMDFastBindingInstance* Binding = Container->AddBinding())
		{
			FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());
			PopulateBindingsList();
			SelectBinding(Binding);
			BindingListView->SetSelection(SelectedBinding);
			NewBinding = SelectedBinding;
		}
	}

	return FReply::Handled();
}

FReply SMDFastBindingEditorWidget::OnDuplicateBinding(TWeakObjectPtr<UMDFastBindingInstance> Binding)
{
	if (UMDFastBindingContainer* Container = GetSelectedBindingContainer())
	{
		FScopedTransaction Transaction = FScopedTransaction(LOCTEXT("DuplicateBindingTransaction", "Duplicated Binding"));
		Container->Modify();

		if (UMDFastBindingInstance* NewBindingPtr = Container->DuplicateBinding(Binding.Get()))
		{
			FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());
			PopulateBindingsList();
			SelectBinding(NewBindingPtr);
			BindingListView->SetSelection(SelectedBinding);
		}
	}

	return FReply::Handled();
}

FReply SMDFastBindingEditorWidget::OnDeleteBinding(TWeakObjectPtr<UMDFastBindingInstance> Binding)
{
	const EAppReturnType::Type ReturnType = FMessageDialog::Open(EAppMsgType::OkCancel, LOCTEXT("DeleteBindingMessage", "Are you sure you want to delete this binding?"));
	if (ReturnType == EAppReturnType::Ok)
	{
		if (UMDFastBindingContainer* Container = GetSelectedBindingContainer())
		{
			FScopedTransaction Transaction = FScopedTransaction(LOCTEXT("DeleteBindingTransaction", "Deleted Binding"));
			Container->Modify();

			if (Container->DeleteBinding(Binding.Get()))
			{
				FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());
				PopulateBindingsList();
				BindingListView->SetSelection(SelectedBinding);
			}
		}
	}

	return FReply::Handled();
}

void SMDFastBindingEditorWidget::OnDetailsPanelPropertyChanged(const FPropertyChangedEvent& Event)
{
	RefreshGraph();

	FBlueprintEditorUtils::MarkBlueprintAsModified(GetBlueprint());
}

void SMDFastBindingEditorWidget::OnBlueprintCompiled(UBlueprint* Blueprint)
{
	const int32 SelectedIndex = Bindings.IndexOfByKey(SelectedBinding);
	AssignBindingData(Blueprint);
	if (Bindings.IsValidIndex(SelectedIndex))
	{
		SelectBinding(Bindings[SelectedIndex].Get());
	}
}

void SMDFastBindingEditorWidget::UpdateBindingBeingDebugged(UObject* ObjectBeingDebugged)
{
	BindingBeingDebugged.Reset();
	if (ObjectBeingDebugged != nullptr)
	{
		// Find the debugged binding container
		if (const UMDFastBindingContainer* Container = MDFastBindingEditorHelpers::FindBindingContainerInObject(ObjectBeingDebugged))
		{
			const int32 SelectedIndex = Bindings.IndexOfByKey(SelectedBinding);
			if (Container->GetBindings().IsValidIndex(SelectedIndex))
			{
				BindingBeingDebugged = Container->GetBindings()[SelectedIndex];
			}
		}
	}

	if (WatchList.IsValid())
	{
		WatchList->SetReferences(GetSelectedBinding(), BindingBeingDebugged.Get());
		WatchList->RefreshList();
	}

	if (BindingGraphWidget.IsValid())
	{
		BindingGraphWidget->SetBindingBeingDebugged(BindingBeingDebugged.Get());
	}
}

void SMDFastBindingEditorWidget::UpdateBindingBeingDebugged()
{
	if (BlueprintEditor.IsValid())
	{
		if (const UBlueprint* Blueprint = BlueprintEditor.Pin()->GetBlueprintObj())
		{
			UpdateBindingBeingDebugged(Blueprint->GetObjectBeingDebugged());
		}
	}
}

FReply SMDFastBindingEditorWidget::OnClearWatches()
{
	if (const UMDFastBindingInstance* Binding = GetSelectedBinding())
	{
		for (const UMDFastBindingObject* Object : Binding->GatherAllBindingObjects())
		{
			if (Object != nullptr)
			{
				UMDFastBindingDebugPersistentData::Get().RemoveNodeFromWatchList(Object->BindingObjectIdentifier);
			}
		}
	}

	return FReply::Handled();
}

UBlueprint* SMDFastBindingEditorWidget::GetBlueprint() const
{
	if (const TSharedPtr<FBlueprintEditor> BPEditor = BlueprintEditor.Pin())
	{
		return BPEditor->GetBlueprintObj();
	}

	return nullptr;
}

const FName FMDFastBindingEditorSummoner::TabId = TEXT("MDFastBindingID");
const FName FMDFastBindingEditorSummoner::DrawerId = TEXT("MDFastBindingDrawerID");

FMDFastBindingEditorSummoner::FMDFastBindingEditorSummoner(TSharedPtr<FBlueprintEditor> BlueprintEditor)
	: FWorkflowTabFactory(TabId, BlueprintEditor)
	, WeakBlueprintEditor(BlueprintEditor)
	, TabIcon(FMDFastBindingEditorStyle::GetStyleSetName(), TEXT("Icon.FastBinding_16x"))
{
	TabLabel = LOCTEXT("BindingEditorTabName", "Binding Editor");
}

const FSlateBrush* FMDFastBindingEditorSummoner::GetTabIcon(const FWorkflowTabSpawnInfo& Info) const
{
	return TabIcon.GetIcon();
}

const FSlateIcon& FMDFastBindingEditorSummoner::GetTabSpawnerIcon(const FWorkflowTabSpawnInfo& Info) const
{
	return TabIcon;
}

TSharedRef<SWidget> FMDFastBindingEditorSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	return SNew(SMDFastBindingEditorWidget, WeakBlueprintEditor);
}

#undef LOCTEXT_NAMESPACE
