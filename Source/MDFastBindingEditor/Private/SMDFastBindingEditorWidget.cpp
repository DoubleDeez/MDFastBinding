#include "SMDFastBindingEditorWidget.h"
#include "BindingDestinations/MDFastBindingDestinationBase.h"
#include "BlueprintEditor.h"
#include "ClassViewerFilter.h"
#include "ClassViewerModule.h"
#include "Kismet2/SClassPickerDialog.h"
#include "MDFastBindingContainer.h"
#include "MDFastBindingEditorDebug.h"
#include "MDFastBindingEditorModule.h"
#include "MDFastBindingEditorPersistantData.h"
#include "MDFastBindingEditorStyle.h"
#include "MDFastBindingGraphNode.h"
#include "MDFastBindingInstance.h"
#include "Misc/MessageDialog.h"
#include "ScopedTransaction.h"
#include "SMDFastBindingEditorGraphWidget.h"
#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"

#define LOCTEXT_NAMESPACE "MDFastBindingEdtiorWidget"

namespace SMDFastBindingEditorWidget_Private
{
	const int32 BindingListIndex = 0;
	const int32 NodeDetailsIndex = 1;
}

/**
 * 
 */
class SBindingRow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SBindingRow)
		{
		}
	SLATE_END_ARGS()

	void StartEditingTitle() const
	{
		if (TitleText.IsValid())
		{
			TitleText->EnterEditingMode();
		}
	}

	void Construct(const FArguments& InArgs, TSharedRef<SMDFastBindingEditorWidget> EditorWidget, TWeakObjectPtr<UMDFastBindingInstance> BindingPtr)
	{
		CachedEditorWidget = EditorWidget;
		CachedBindingPtr = BindingPtr;
		
		if (UMDFastBindingInstance* Binding = BindingPtr.Get())
		{
			ChildSlot
			[
				SNew(SBorder)
				.Padding(3.f)
				.BorderImage(this, &SBindingRow::GetBorder)
				[
					SNew(SHorizontalBox)
					+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Fill)
					.AutoWidth()
					.Padding(4.f, 0.f)
					[
						SNew(SImage)
						.ToolTipText(EditorWidget, &SMDFastBindingEditorWidget::GetBindingValidationTooltip, BindingPtr)
						.Image(EditorWidget, &SMDFastBindingEditorWidget::GetBindingValidationBrush, BindingPtr)
					]
					+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Fill)
					.AutoWidth()
					.Padding(4.f, 0.f)
					[
						SNew(SImage)
						.ToolTipText(EditorWidget, &SMDFastBindingEditorWidget::GetBindingPerformanceTooltip, BindingPtr)
						.Image(EditorWidget, &SMDFastBindingEditorWidget::GetBindingPerformanceBrush, BindingPtr)
					]
					+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Fill)
					.FillWidth(1.f)
					[
						SAssignNew(TitleText, SInlineEditableTextBlock)
						.Text_UObject(Binding, &UMDFastBindingInstance::GetBindingDisplayName)
						.OnTextCommitted(EditorWidget, &SMDFastBindingEditorWidget::SetBindingDisplayName, BindingPtr)
					]
					+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Fill)
					.AutoWidth()
					.Padding(2.f)
					[
						SNew(SButton)
						.ButtonStyle(FMDFastBindingEditorStyle::Get(), "BindingButton")
						.ContentPadding(2.f)
						.OnClicked(EditorWidget, &SMDFastBindingEditorWidget::OnDuplicateBinding, BindingPtr)
						.ToolTipText(LOCTEXT("DuplicateBindingTooltip", "Create a duplicate of this binding"))
						[
							SNew(SImage)
							.Image(FAppStyle::Get().GetBrush(TEXT("GenericCommands.Duplicate")))
						]
					]
					+SHorizontalBox::Slot()
					.VAlign(VAlign_Center)
					.HAlign(HAlign_Fill)
					.AutoWidth()
					.Padding(2.f)
					[
						SNew(SButton)
						.ButtonStyle(FMDFastBindingEditorStyle::Get(), "BindingButton")
						.ContentPadding(2.f)
						.OnClicked(EditorWidget, &SMDFastBindingEditorWidget::OnDeleteBinding, BindingPtr)
						.ToolTipText(LOCTEXT("DeleteBindingTooltip", "Delete this binding"))
						[
							SNew(SImage)
							.Image(FAppStyle::Get().GetBrush(TEXT("GenericCommands.Delete")))
						]
					]
				]
			];
		}
	}

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override
	{
		// Focus the newly created binding's name field
		if (CachedEditorWidget.Pin().IsValid() && CachedEditorWidget.Pin()->NewBinding == CachedBindingPtr)
		{
			StartEditingTitle();
			CachedEditorWidget.Pin()->NewBinding.Reset();
		}
	}

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
		{
			if (CachedEditorWidget.Pin().IsValid())
			{
				CachedEditorWidget.Pin()->SelectBinding(CachedBindingPtr.Get());
			}
		}

		return SCompoundWidget::OnMouseButtonDown(MyGeometry, MouseEvent);
	}

private:
	const FSlateBrush* GetBorder() const
	{
		if (CachedEditorWidget.Pin().IsValid() && CachedEditorWidget.Pin()->SelectedBinding == CachedBindingPtr)
     	{
			return FMDFastBindingEditorStyle::Get().GetBrush(TEXT("Background.Selector"));
     	}

		return FMDFastBindingEditorStyle::Get().GetBrush(TEXT("Background.SelectorInactive"));
	}
	
	TSharedPtr<SInlineEditableTextBlock> TitleText;
	TWeakObjectPtr<UMDFastBindingInstance> CachedBindingPtr;
	TWeakPtr<SMDFastBindingEditorWidget> CachedEditorWidget;
};


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
	AssignBindingData(Blueprint->GeneratedClass);
	Blueprint->OnCompiled().AddSP(this, &SMDFastBindingEditorWidget::OnBlueprintCompiled);
	Blueprint->OnSetObjectBeingDebugged().AddSP(this, &SMDFastBindingEditorWidget::UpdateBindingBeingDebugged, Blueprint);

	
	BindingListView = SNew(SListView<TWeakObjectPtr<UMDFastBindingInstance>>)
		.ListItemsSource(&Bindings)
		.SelectionMode(ESelectionMode::None)
		.OnGenerateRow(this, &SMDFastBindingEditorWidget::GenerateBindingListWidget);

	BindingListView->SetSelection(GetSelectedBinding());

	BindingGraphWidget = SNew(SMDFastBindingEditorGraphWidget, Blueprint)
		.OnSelectionChanged(this, &SMDFastBindingEditorWidget::OnGraphSelectionChanged);
	BindingGraphWidget->SetBinding(GetSelectedBinding());
	
	UpdateBindingBeingDebugged(Blueprint->GetObjectBeingDebugged(), Blueprint);
	
	WatchList = SNew(SMDFastBindingWatchList);
	WatchList->SetReferences(GetSelectedBinding(), BindingBeingDebugged.Get());

	UMDFastBindingEditorPersistantData::Get().OnWatchListChanged.AddSP(WatchList.Get(), &SMDFastBindingWatchList::RefreshList);
	
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

void SMDFastBindingEditorWidget::AssignBindingData(UClass* BindingOwnerClass)
{
	BindingContainerProperty.Reset();
	BindingContainer.Reset();

	if (BindingOwnerClass != nullptr)
	{
		for (TFieldIterator<FObjectPropertyBase> It(BindingOwnerClass); It; ++It)
		{
			if (It->PropertyClass->IsChildOf(UMDFastBindingContainer::StaticClass()))
			{
				if (UObject* BindingOwnerCDO = BindingOwnerClass->GetDefaultObject())
				{
					BindingContainerProperty = *It;
					if (UMDFastBindingContainer* Container = Cast<UMDFastBindingContainer>(It->GetObjectPropertyValue_InContainer(BindingOwnerCDO)))
					{
						BindingContainer = Container;
					}
					else
					{
						UMDFastBindingContainer* NewContainer = NewObject<UMDFastBindingContainer>(BindingOwnerCDO, NAME_None, RF_Transactional | RF_Public);
						It->SetObjectPropertyValue_InContainer(BindingOwnerCDO, NewContainer);
						BindingOwnerCDO->Modify();
						BindingContainer = NewContainer;
					}
				}
			}
		}
	}
	
	PopulateBindingsList();
}

void SMDFastBindingEditorWidget::SelectBinding(UMDFastBindingInstance* InBinding)
{
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

EVisibility SMDFastBindingEditorWidget::GetBindingSelectorVisibility() const
{
	return (GetSelectedBindingContainer() != nullptr) ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SMDFastBindingEditorWidget::GetBindingTreeVisibility() const
{
	return (GetSelectedBindingContainer() != nullptr && SelectedBinding.IsValid()) ? EVisibility::Visible : EVisibility::Collapsed;
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

TSharedRef<ITableRow> SMDFastBindingEditorWidget::GenerateBindingListWidget(TWeakObjectPtr<UMDFastBindingInstance> Binding, const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STableRow<TWeakObjectPtr<UMDFastBindingInstance>>, OwnerTable)
		[
			SNew(SBindingRow, SharedThis(this), Binding)
		];
}

void SMDFastBindingEditorWidget::SetBindingDisplayName(const FText& InName, ETextCommit::Type CommitType, TWeakObjectPtr<UMDFastBindingInstance> Binding)
{
	if (UMDFastBindingInstance* BindingPtr = Binding.Get())
	{
		FScopedTransaction Transaction = FScopedTransaction(LOCTEXT("RenameBindingTransaction", "Renamed Binding"));
		BindingPtr->SetBindingDisplayName(InName);
	}
}

FReply SMDFastBindingEditorWidget::OnAddBinding()
{		
	if (UMDFastBindingContainer* Container = GetSelectedBindingContainer())
	{
		FScopedTransaction Transaction = FScopedTransaction(LOCTEXT("AddBindingTransaction", "Added Binding"));
		Container->Modify();
		
		if (UMDFastBindingInstance* Binding = Container->AddBinding())
		{
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
}

FText SMDFastBindingEditorWidget::GetBindingValidationTooltip(TWeakObjectPtr<UMDFastBindingInstance> Binding) const
{
	if (UMDFastBindingInstance* BindingPtr = Binding.Get())
	{
		TArray<FText> Errors;
		BindingPtr->IsDataValid(Errors);
		return FText::Join(FText::FromString(TEXT("\n")), Errors);
	}

	return FText::GetEmpty();
}

const FSlateBrush* SMDFastBindingEditorWidget::GetBindingValidationBrush(TWeakObjectPtr<UMDFastBindingInstance> Binding) const
{
	if (UMDFastBindingInstance* BindingPtr = Binding.Get())
	{
		TArray<FText> Errors;
		const EDataValidationResult Result = BindingPtr->IsDataValid(Errors);
		if (Result == EDataValidationResult::Valid)
		{
			return FAppStyle::Get().GetBrush("Icons.SuccessWithColor");
		}
		else if (Result == EDataValidationResult::Invalid)
		{
			return FAppStyle::Get().GetBrush("Icons.ErrorWithColor");
		}
	}

	return nullptr;
}

FText SMDFastBindingEditorWidget::GetBindingPerformanceTooltip(TWeakObjectPtr<UMDFastBindingInstance> Binding) const
{
	if (const UMDFastBindingInstance* BindingPtr = Binding.Get())
	{
		if (BindingPtr->IsBindingPerformant())
		{
			return LOCTEXT("PerformantBindingTooltip", "This binding does not run every tick");
		}
		else
		{
			return LOCTEXT("PerformantBindingTooltip", "This binding is evaluated every tick");
		}
	}
	
	return FText::GetEmpty();
}

const FSlateBrush* SMDFastBindingEditorWidget::GetBindingPerformanceBrush(TWeakObjectPtr<UMDFastBindingInstance> Binding) const
{
	if (const UMDFastBindingInstance* BindingPtr = Binding.Get())
	{
		if (BindingPtr->IsBindingPerformant())
		{
			return FMDFastBindingEditorStyle::Get().GetBrush("Icon.Flame");
		}
		else
		{
			return FMDFastBindingEditorStyle::Get().GetBrush("Icon.Clock");
		}
	}
	
	return nullptr;
}

void SMDFastBindingEditorWidget::OnBlueprintCompiled(UBlueprint* Blueprint)
{
	const int32 SelectedIndex = Bindings.IndexOfByKey(SelectedBinding);
	AssignBindingData(Blueprint != nullptr ? Blueprint->GeneratedClass : nullptr);
	if (Bindings.IsValidIndex(SelectedIndex))
	{
		SelectBinding(Bindings[SelectedIndex].Get());
	}
}

void SMDFastBindingEditorWidget::UpdateBindingBeingDebugged(UObject* ObjectBeingDebugged, UBlueprint* Blueprint)
{
	BindingBeingDebugged.Reset();
	if (Blueprint != nullptr && ObjectBeingDebugged != nullptr && ObjectBeingDebugged->GetClass() == Blueprint->GeneratedClass && BindingContainerProperty.IsValid())
	{
		// Find the debugged binding container
		if (UMDFastBindingContainer* Container = Cast<UMDFastBindingContainer>(BindingContainerProperty->GetObjectPropertyValue_InContainer(ObjectBeingDebugged)))
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
		if (UBlueprint* Blueprint = BlueprintEditor.Pin()->GetBlueprintObj())
		{
			UpdateBindingBeingDebugged(Blueprint->GetObjectBeingDebugged(), Blueprint);
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
				UMDFastBindingEditorPersistantData::Get().RemoveNodeFromWatchList(Object->BindingObjectIdentifier);
			}
		}
	}
	
	return FReply::Handled();
}

const FName FMDFastBindingEditorSummoner::TabId = TEXT("MDFastBindingID");

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
