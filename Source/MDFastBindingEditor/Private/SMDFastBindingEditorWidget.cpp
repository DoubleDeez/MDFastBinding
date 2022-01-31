#include "SMDFastBindingEditorWidget.h"
#include "BlueprintEditor.h"
#include "ClassViewerFilter.h"
#include "ClassViewerModule.h"
#include "MDFastBindingContainer.h"
#include "MDFastBindingEditorModule.h"
#include "MDFastBindingEditorStyle.h"
#include "MDFastBindingGraphNode.h"
#include "SMDFastBindingEditorGraphWidget.h"
#include "BindingDestinations/MDFastBindingDestinationBase.h"
#include "Kismet2/SClassPickerDialog.h"
#include "Widgets/Layout/SWidgetSwitcher.h"

#define LOCTEXT_NAMESPACE "MDFastBindingEdtiorWidget"

namespace SMDFastBindingEditorWidget_Private
{
	const int32 BindingListIndex = 0;
	const int32 NodeDetailsIndex = 1;
	
	void GatherNodeChildren(const void* NodeValue, const UStruct* NodeStruct, TArray<TSharedRef<FMDBindingEditorContainerSelectMenuNode>>& OutChildren, TArray<TWeakObjectPtr<UMDFastBindingContainer>>& OutBindingContainers)
	{
		if (!FMDFastBindingEditorModule::DoesClassHaveFastBindings(NodeStruct))
		{
			return;
		}

		auto CheckProperty = [&](const void* ChildValuePtr, UStruct* ChildStruct)
		{
			if (ChildValuePtr != nullptr)
			{
				TArray<TSharedRef<FMDBindingEditorContainerSelectMenuNode>> Children;
				GatherNodeChildren(ChildValuePtr, ChildStruct, Children, OutBindingContainers);
				if (Children.Num() > 0)
				{
					TSharedRef<FMDBindingEditorContainerSelectMenuNode> Node = MakeShared<FMDBindingEditorContainerSelectMenuNode>();
					Node->NodeClass = ChildStruct;
					Node->Children = MoveTemp(Children);
					OutChildren.Emplace(MoveTemp(Node));
				}	
			}
		};

		for (TFieldIterator<FProperty> It(NodeStruct); It; ++It)
		{
			if (const FObjectPropertyBase* ObjectProp = CastField<const FObjectPropertyBase>(*It))
			{
				if (ObjectProp->PropertyClass->IsChildOf(UMDFastBindingContainer::StaticClass()))
				{
					TSharedRef<FMDBindingEditorContainerSelectMenuNode> Node = MakeShared<FMDBindingEditorContainerSelectMenuNode>();
					Node->NodeClass = ObjectProp->PropertyClass;
					const bool bOuterIsUObject = Cast<const UClass>(NodeStruct) != nullptr;
					Node->BindingContainer = (bOuterIsUObject)
						? Cast<UMDFastBindingContainer>(ObjectProp->GetObjectPropertyValue_InContainer(*static_cast<UObject* const*>(NodeValue)))
						: Cast<UMDFastBindingContainer>(ObjectProp->GetObjectPropertyValue_InContainer(NodeValue));

					if (Node->BindingContainer.IsValid())
					{
						Node->BindingContainer->SetFlags(RF_Transactional);
					}
					else if (bOuterIsUObject)
					{
						UMDFastBindingContainer* NewContainer = NewObject<UMDFastBindingContainer>(*static_cast<UObject* const*>(NodeValue), NAME_None, RF_Public | RF_Transactional);
						ObjectProp->SetObjectPropertyValue_InContainer(*static_cast<UObject* const*>(NodeValue), NewContainer);
						Node->BindingContainer = NewContainer;
					}
					else
					{
						// TODO - Init container if outer is struct (we need the outer UObject)
					}
					
					OutBindingContainers.Add(Node->BindingContainer);
					OutChildren.Emplace(MoveTemp(Node));
				}
				else
				{
					const void* ChildValuePtr = (Cast<const UClass>(NodeStruct) != nullptr)
						? ObjectProp->ContainerPtrToValuePtr<void>(*static_cast<UObject* const*>(NodeValue))
						: ObjectProp->ContainerPtrToValuePtr<void>(NodeValue);

					CheckProperty(ChildValuePtr, ObjectProp->PropertyClass);
				}
			}
			else if (const FStructProperty* StructProp = CastField<const FStructProperty>(*It))
			{
				const void* ChildValuePtr = (Cast<const UClass>(NodeStruct) != nullptr)
					? StructProp->ContainerPtrToValuePtr<void>(*static_cast<UObject* const*>(NodeValue))
					: StructProp->ContainerPtrToValuePtr<void>(NodeValue);

				CheckProperty(ChildValuePtr, StructProp->Struct);
			}
		}
	}
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
	FDetailsViewArgs DetailsViewArgs;
	DetailsViewArgs.bLockable = false;
	DetailsViewArgs.bShowPropertyMatrixButton = false;
	DetailsViewArgs.bHideSelectionTip = true;
	DetailsView = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor").CreateDetailView(DetailsViewArgs);
	DetailsView->OnFinishedChangingProperties().AddSP(this, &SMDFastBindingEditorWidget::OnDetailsPanelPropertyChanged);
	
	FMenuBarBuilder MenuBuilder = FMenuBarBuilder(nullptr);
	MenuBuilder.AddPullDownMenu(
		LOCTEXT( "SelectMenu", "Select Binding container" ),
		LOCTEXT( "SelectMenu_ToolTip", "Select which binding container to edit" ),
		FNewMenuDelegate::CreateSP(this, &SMDFastBindingEditorWidget::FillSelectMenu_Root));
	
	const TSharedRef<SWidget> MenuBarWidget = MenuBuilder.MakeWidget();
	MenuBarWidget->SetVisibility(TAttribute<EVisibility>(this, &SMDFastBindingEditorWidget::GetContainerSelectorVisibility));
	
	AssignBindingData(InBlueprintEditor.Pin()->GetBlueprintObj()->GeneratedClass);
	SelectBindingContainer(nullptr);
	
	BindingListView = SNew(SListView<TWeakObjectPtr<UMDFastBindingDestinationBase>>)
		.ListItemsSource(&Bindings)
		.SelectionMode(ESelectionMode::Single)
		.OnGenerateRow(this, &SMDFastBindingEditorWidget::GenerateBindingListWidget)
		.OnSelectionChanged(this, &SMDFastBindingEditorWidget::SelectBinding);

	BindingListView->SetSelection(GetSelectedBinding());

	BindingGraphWidget = SNew(SMDFastBindingEditorGraphWidget)
		.OnSelectionChanged(this, &SMDFastBindingEditorWidget::OnGraphSelectionChanged);
	BindingGraphWidget->SetBinding(GetSelectedBinding());
	
	ChildSlot
	[
		SNew(SSplitter)
		+SSplitter::Slot()
#if ENGINE_MAJOR_VERSION > 4
		.MinSize(350.f)
#endif
		.Value(0.15f)
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
					MenuBarWidget
				]
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
#if ENGINE_MAJOR_VERSION <= 4
						.Image(FCoreStyle::Get().GetBrush(TEXT("EditableComboBox.Add")))
#else
						.Image(FAppStyle::Get().GetBrush(TEXT("EditableComboBox.Add")))
#endif
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
	BindingContainers.Empty();

	if (BindingOwnerClass != nullptr)
	{
		RootBindingNode = MakeShared<FMDBindingEditorContainerSelectMenuNode>();
		RootBindingNode->NodeClass = BindingOwnerClass;
		const UObject* BindingOwnerCDO = BindingOwnerClass->GetDefaultObject();
		SMDFastBindingEditorWidget_Private::GatherNodeChildren(&BindingOwnerCDO, BindingOwnerClass, RootBindingNode->Children, BindingContainers);
	}
}

void SMDFastBindingEditorWidget::SelectBindingContainer(UMDFastBindingContainer* BindingContainer)
{
	SelectedBindingContainer = BindingContainer;
	PopulateBindingsList();
}

void SMDFastBindingEditorWidget::SelectBindingContainer(TWeakObjectPtr<UMDFastBindingContainer> BindingContainer)
{
	SelectBindingContainer(BindingContainer.Get());
}

void SMDFastBindingEditorWidget::SelectBinding(UMDFastBindingDestinationBase* InBinding)
{
	SelectedBinding = InBinding;
	if (BindingGraphWidget.IsValid())
	{
		BindingGraphWidget->SetBinding(GetSelectedBinding());
	}
}

void SMDFastBindingEditorWidget::SelectBinding(TWeakObjectPtr<UMDFastBindingDestinationBase> InBinding, ESelectInfo::Type SelectionType)
{
	SelectBinding(InBinding.Get());
}

UMDFastBindingContainer* SMDFastBindingEditorWidget::GetSelectedBindingContainer() const
{
	if (SelectedBindingContainer.IsValid())
	{
		return SelectedBindingContainer.Get();
	}

	return (BindingContainers.Num() > 0) ? BindingContainers[0].Get() : nullptr;
}

UMDFastBindingDestinationBase* SMDFastBindingEditorWidget::GetSelectedBinding() const
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
}

void SMDFastBindingEditorWidget::PostRedo(bool bSuccess)
{
	PopulateBindingsList();
	BindingListView->SetSelection(SelectedBinding);
}

void SMDFastBindingEditorWidget::FillSelectMenu_Root(FMenuBuilder& MenuBuilder)
{
	const FText DisplayText = RootBindingNode->DisplayName.IsEmpty()
		? RootBindingNode->NodeClass->GetDisplayNameText()
		: RootBindingNode->DisplayName;
	MenuBuilder.AddSubMenu(
		DisplayText,
		FText::GetEmpty(),
		FNewMenuDelegate::CreateSP(this, &SMDFastBindingEditorWidget::FillSelectMenu, RootBindingNode.ToSharedRef()));
}

void SMDFastBindingEditorWidget::FillSelectMenu(FMenuBuilder& MenuBuilder, TSharedRef<FMDBindingEditorContainerSelectMenuNode> BindingNode)
{
	for (const TSharedRef<FMDBindingEditorContainerSelectMenuNode>& ChildNode : BindingNode->Children)
	{
		const FText DisplayText = ChildNode->DisplayName.IsEmpty()
			? ChildNode->NodeClass->GetDisplayNameText()
			: ChildNode->DisplayName;
		if (ChildNode->NodeClass->IsChildOf<UMDFastBindingContainer>())
		{
			const bool bIsSelectedContainer = GetSelectedBindingContainer() == ChildNode->BindingContainer.Get();
			MenuBuilder.AddMenuEntry(
				DisplayText,
				FText::GetEmpty(),
				bIsSelectedContainer ? FSlateIcon(FMDFastBindingEditorStyle::GetStyleSetName(), TEXT("Icon.Check")) : FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateSP(this, &SMDFastBindingEditorWidget::SelectBindingContainer, ChildNode->BindingContainer),
					FCanExecuteAction()
				),
				NAME_None,
				EUserInterfaceActionType::Button
			);
		}
		else
		{
			MenuBuilder.AddSubMenu(
				DisplayText,
				FText::GetEmpty(),
				FNewMenuDelegate::CreateSP(this, &SMDFastBindingEditorWidget::FillSelectMenu, ChildNode));
		}
	}
}

void SMDFastBindingEditorWidget::OnGraphSelectionChanged(const FGraphPanelSelectionSet& Selection)
{
	if (DetailSwitcher.IsValid())
	{
		if (Selection.Num() == 0)
		{
			DetailSwitcher->SetActiveWidgetIndex(SMDFastBindingEditorWidget_Private::BindingListIndex);
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

EVisibility SMDFastBindingEditorWidget::GetContainerSelectorVisibility() const
{
	return (BindingContainers.Num() > 1) ? EVisibility::Visible : EVisibility::Collapsed;
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
		Bindings = TArray<TWeakObjectPtr<UMDFastBindingDestinationBase>>(Container->GetBindings());
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

TSharedRef<ITableRow> SMDFastBindingEditorWidget::GenerateBindingListWidget(TWeakObjectPtr<UMDFastBindingDestinationBase> Binding, const TSharedRef<STableViewBase>& OwnerTable)
{
	if (UMDFastBindingDestinationBase* BindingPtr = Binding.Get())
	{
		return SNew(STableRow<TWeakObjectPtr<UMDFastBindingDestinationBase>>, OwnerTable)
		.Padding(3.f)
		[
			SNew(SHorizontalBox)
			+SHorizontalBox::Slot()
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Fill)
			.FillWidth(1.f)
			[
				SNew(STextBlock)
				.Text_UObject(BindingPtr, &UMDFastBindingObject::GetDisplayName)
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
				.OnClicked(this, &SMDFastBindingEditorWidget::OnDuplicateBinding, Binding)
				.ToolTipText(LOCTEXT("DuplicateBindingTooltip", "Create a duplicate of this binding"))
				[
					SNew(SImage)
#if ENGINE_MAJOR_VERSION <= 4
					.Image(FCoreStyle::Get().GetBrush(TEXT("GenericCommands.Duplicate")))
#else
					.Image(FAppStyle::Get().GetBrush(TEXT("GenericCommands.Duplicate")))
#endif
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
				.OnClicked(this, &SMDFastBindingEditorWidget::OnDeleteBinding, Binding)
				.ToolTipText(LOCTEXT("DeleteBindingTooltip", "Delete this binding"))
				[
					SNew(SImage)
#if ENGINE_MAJOR_VERSION <= 4
					.Image(FCoreStyle::Get().GetBrush(TEXT("GenericCommands.Delete")))
#else
					.Image(FAppStyle::Get().GetBrush(TEXT("GenericCommands.Delete")))
#endif
				]
			]
		];
	}
	
	return SNew(STableRow<TWeakObjectPtr<UMDFastBindingDestinationBase>>, OwnerTable)
	[
		SNullWidget::NullWidget
	];
}

FReply SMDFastBindingEditorWidget::OnAddBinding()
{	
	FClassViewerInitializationOptions Options;
	Options.Mode = EClassViewerMode::ClassPicker;
	Options.DisplayMode = EClassViewerDisplayMode::ListView;
	Options.NameTypeToDisplay = EClassViewerNameTypeToDisplay::Dynamic;
	Options.ClassFilter = MakeShareable(new FMDFastBindingDestinationClassFilter);
	
	UClass* ChosenClass = nullptr;
	const bool bPressedOk = SClassPickerDialog::PickClass(LOCTEXT("ClassPickerTitle", "Select Binding Destination..."), Options, ChosenClass, UMDFastBindingDestinationBase::StaticClass());
	if (bPressedOk && ChosenClass != nullptr)
	{
		if (UMDFastBindingContainer* Container = GetSelectedBindingContainer())
		{
			FScopedTransaction Transaction = FScopedTransaction(LOCTEXT("AddBindingTransaction", "Added Binding Destination"));
			Container->Modify();
			
			if (UMDFastBindingDestinationBase* Binding = Container->AddBinding(ChosenClass))
			{
				PopulateBindingsList();
				SelectBinding(Binding);
				BindingListView->SetSelection(SelectedBinding);
			}
		}
	}

	return FReply::Handled();
}

FReply SMDFastBindingEditorWidget::OnDuplicateBinding(TWeakObjectPtr<UMDFastBindingDestinationBase> Binding)
{
	if (UMDFastBindingContainer* Container = GetSelectedBindingContainer())
	{
		FScopedTransaction Transaction = FScopedTransaction(LOCTEXT("DuplicateBindingTransaction", "Duplicated Binding"));
		Container->Modify();
			
		if (UMDFastBindingDestinationBase* NewBinding = Container->DuplicateBinding(Binding.Get()))
		{
			PopulateBindingsList();
			SelectBinding(NewBinding);
			BindingListView->SetSelection(SelectedBinding);
		}
	}
	
	return FReply::Handled();
}

FReply SMDFastBindingEditorWidget::OnDeleteBinding(TWeakObjectPtr<UMDFastBindingDestinationBase> Binding)
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
