#include "SMDFastBindingEditorGraphWidget.h"

#include "EdGraphUtilities.h"
#include "MDFastBindingEditorCommands.h"
#include "MDFastBindingGraph.h"
#include "MDFastBindingGraphNode.h"
#include "MDFastBindingGraphSchema.h"
#include "MDFastBindingInstance.h"
#include "MDFastBindingObject.h"
#include "SGraphActionMenu.h"
#include "Algo/Transform.h"
#include "BindingDestinations/MDFastBindingDestinationBase.h"
#include "BindingValues/MDFastBindingValueBase.h"
#include "Framework/Commands/GenericCommands.h"
#include "HAL/PlatformApplicationMisc.h"
#include "UObject/StrongObjectPtr.h"

#define LOCTEXT_NAMESPACE "SMDFastBindingEditorGraphWidget"

SMDFastBindingEditorGraphWidget::~SMDFastBindingEditorGraphWidget()
{
	GraphObj->RemoveFromRoot();
}

void SMDFastBindingEditorGraphWidget::Construct(const FArguments& InArgs, UBlueprint* Blueprint)
{
	GraphObj = NewObject<UMDFastBindingGraph>(Blueprint);
	GraphObj->AddToRoot();
	GraphObj->Schema = UMDFastBindingGraphSchema::StaticClass();
	GraphObj->SetGraphWidget(SharedThis(this));
	
	SGraphEditor::FGraphEditorEvents GraphEvents;
	GraphEvents.OnTextCommitted.BindSP(this, &SMDFastBindingEditorGraphWidget::OnNodeTitleChanged);
	GraphEvents.OnSelectionChanged = InArgs._OnSelectionChanged;
	GraphEvents.OnCreateActionMenu.BindSP(this, &SMDFastBindingEditorGraphWidget::OnCreateActionMenu);
	GraphEvents.OnCreateNodeOrPinMenu.BindSP(this, &SMDFastBindingEditorGraphWidget::OnCreateNodeOrPinMenu);

	RegisterCommands();
	
	GraphEditor = SNew(SGraphEditor)
		.GraphToEdit(GraphObj)
		.AdditionalCommands(GraphEditorCommands)
		.GraphEvents(GraphEvents);
	
	ChildSlot
	[
		GraphEditor.ToSharedRef()
	];
}

void SMDFastBindingEditorGraphWidget::RegisterCommands()
{
	if (GraphEditorCommands.IsValid())
	{
		return;
	}

	FMDFastBindingEditorCommands::Register();

	GraphEditorCommands = MakeShared<FUICommandList>();

	GraphEditorCommands->MapAction(FGenericCommands::Get().Delete,
		FExecuteAction::CreateSP(this, &SMDFastBindingEditorGraphWidget::DeleteSelectedNodes),
		FCanExecuteAction::CreateSP(this, &SMDFastBindingEditorGraphWidget::CanDeleteSelectedNodes)
		);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Rename,
		FExecuteAction::CreateSP(this, &SMDFastBindingEditorGraphWidget::RenameSelectedNode),
		FCanExecuteAction()
		);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Copy,
		FExecuteAction::CreateSP(this, &SMDFastBindingEditorGraphWidget::CopySelectedNodes),
		FCanExecuteAction()
		);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Paste,
		FExecuteAction::CreateSP(this, &SMDFastBindingEditorGraphWidget::PasteNodes),
		FCanExecuteAction::CreateSP(this, &SMDFastBindingEditorGraphWidget::CanPasteNodes)
		);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Cut,
		FExecuteAction::CreateSP(this, &SMDFastBindingEditorGraphWidget::CutSelectedNodes),
		FCanExecuteAction::CreateSP(this, &SMDFastBindingEditorGraphWidget::CanCutSelectedNodes)
		);

	GraphEditorCommands->MapAction(FMDFastBindingEditorCommands::Get().SetDestinationActive,
		FExecuteAction::CreateSP(this, &SMDFastBindingEditorGraphWidget::SetDestinationActive),
		FCanExecuteAction::CreateSP(this, &SMDFastBindingEditorGraphWidget::CanSetDestinationActive)
		);
}

void SMDFastBindingEditorGraphWidget::SetBinding(UMDFastBindingInstance* InBinding)
{
	Binding = InBinding;
	GraphObj->SetBinding(InBinding);
}

void SMDFastBindingEditorGraphWidget::RefreshGraph() const
{
	GraphObj->RefreshGraph();
}

void SMDFastBindingEditorGraphWidget::SetSelection(UEdGraphNode* InNode, bool bIsSelected)
{
	GraphEditor->SetNodeSelection(InNode, bIsSelected);
}

void SMDFastBindingEditorGraphWidget::ClearSelection()
{
	GraphEditor->ClearSelectionSet();
}

void SMDFastBindingEditorGraphWidget::OnNodeTitleChanged(const FText& InText, ETextCommit::Type Type,
                                                         UEdGraphNode* Node)
{
	if (Node != nullptr)
	{
		Node->OnRenameNode(InText.ToString());
	}
}

const TArray<TSubclassOf<UMDFastBindingValueBase>>& SMDFastBindingEditorGraphWidget::GetValueClasses()
{
	if (ValueClasses.Num() == 0)
	{
		for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
		{
			if (ClassIt->IsChildOf(UMDFastBindingValueBase::StaticClass()) && !ClassIt->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_Hidden))
			{
				ValueClasses.Add(*ClassIt);
			}
		}

		ValueClasses.Sort([](const TSubclassOf<UMDFastBindingValueBase>& A, const TSubclassOf<UMDFastBindingValueBase>& B)
		{
			return A->GetDisplayNameText().CompareTo(B->GetDisplayNameText()) < 0;
		});
	}

	return ValueClasses;
}

const TArray<TSubclassOf<UMDFastBindingDestinationBase>>& SMDFastBindingEditorGraphWidget::GetDestinationClasses()
{
	if (DestinationClasses.Num() == 0)
	{
		for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
		{
			if (ClassIt->IsChildOf(UMDFastBindingDestinationBase::StaticClass()) && !ClassIt->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_Hidden))
			{
				DestinationClasses.Add(*ClassIt);
			}
		}

		DestinationClasses.Sort([](const TSubclassOf<UMDFastBindingDestinationBase>& A, const TSubclassOf<UMDFastBindingDestinationBase>& B)
		{
			return A->GetDisplayNameText().CompareTo(B->GetDisplayNameText()) < 0;
		});
	}

	return DestinationClasses;
}

FActionMenuContent SMDFastBindingEditorGraphWidget::OnCreateActionMenu(UEdGraph* InGraph, const FVector2D& InNodePosition, const TArray<UEdGraphPin*>& InDraggedPins, bool bAutoExpand, SGraphEditor::FActionMenuClosed InOnMenuClose)
{
	const TSharedRef<SGraphActionMenu> ActionMenu = SNew(SGraphActionMenu)
						.OnActionSelected(this, &SMDFastBindingEditorGraphWidget::OnActionSelected, InNodePosition, InDraggedPins)
						.OnCollectAllActions(this, &SMDFastBindingEditorGraphWidget::CollectAllActions, InDraggedPins)
						.DraggedFromPins(InDraggedPins)
						.GraphObj(GraphObj);
	
	return FActionMenuContent(
		SNew(SBorder)
		.BorderImage(FAppStyle::GetBrush("Menu.Background"))
		[
			SNew(SBox)
			.MinDesiredWidth(300.f)
			.MinDesiredHeight(400.f)
			[
				ActionMenu
			]
		]);
}

FActionMenuContent SMDFastBindingEditorGraphWidget::OnCreateNodeOrPinMenu(UEdGraph* CurrentGraph, const UEdGraphNode* InGraphNode, const UEdGraphPin* InGraphPin, FMenuBuilder* MenuBuilder, bool bIsDebugging)
{
	// Menu when right clicking

	if (const UMDFastBindingGraphNode* GraphNode = Cast<UMDFastBindingGraphNode>(InGraphNode))
	{
		MenuBuilder->AddMenuEntry(FGenericCommands::Get().Rename);
		MenuBuilder->AddMenuEntry(FGenericCommands::Get().Delete);
		
		if (UMDFastBindingObject* BindingObject = GraphNode->GetBindingObject())
		{
			if (const UMDFastBindingDestinationBase* BindingDest = Cast<UMDFastBindingDestinationBase>(BindingObject))
			{
				if (!BindingDest->IsActive())
				{
					MenuBuilder->AddMenuEntry(FMDFastBindingEditorCommands::Get().SetDestinationActive);
				}
			}

			if (InGraphPin != nullptr)
			{
				if (const FMDFastBindingItem* PinItem = BindingObject->FindBindingItem(InGraphPin->GetFName()))
				{
					if (PinItem->ExtendablePinListIndex != INDEX_NONE)
					{
						MenuBuilder->AddMenuEntry(
							LOCTEXT("RemovePin", "Remove pin"),
							LOCTEXT("RemovePinTooltip", "Remove this input pin (and any accompanying pins with the same index)"),
							FSlateIcon(),
							FUIAction(
								FExecuteAction::CreateSP(this, &SMDFastBindingEditorGraphWidget::RemoveExtendablePin, MakeWeakObjectPtr(BindingObject), PinItem->ExtendablePinListIndex)
							)
						);
					}
				}
			}
		}
	}
    	
	return FActionMenuContent(MenuBuilder->MakeWidget());
}

void SMDFastBindingEditorGraphWidget::OnActionSelected(const TArray<TSharedPtr<FEdGraphSchemaAction>>& SelectedActions, ESelectInfo::Type InSelectionType, FVector2D InNodePosition, TArray<UEdGraphPin*> InDraggedPins) const
{
	if (InSelectionType == ESelectInfo::OnMouseClick  || InSelectionType == ESelectInfo::OnKeyPress)
	{
		for (TSharedPtr<FEdGraphSchemaAction> SelectedAction : SelectedActions)
		{
			if (SelectedAction.IsValid() && GraphObj != nullptr)
			{
				SelectedAction->PerformAction(GraphObj, InDraggedPins.Num() > 0 ? InDraggedPins[0] : nullptr, InNodePosition);
			}
		}
	}

	FSlateApplication::Get().DismissAllMenus();
}

void SMDFastBindingEditorGraphWidget::CollectAllActions(FGraphActionListBuilderBase& OutAllActions, TArray<UEdGraphPin*> InDraggedPins)
{
	static const FString CreateValueCategory = TEXT("Create Value Node...");
	for (const TSubclassOf<UMDFastBindingValueBase>& ValueClass : GetValueClasses())
	{
		OutAllActions.AddAction(MakeShared<FMDFastBindingSchemaAction_CreateValue>(ValueClass), CreateValueCategory);
	}
	
	if (InDraggedPins.Num() == 0 || (InDraggedPins.Num() == 1 && InDraggedPins[0] != nullptr && InDraggedPins[0]->Direction == EGPD_Output))
	{
		static const FString SetDestinationCategory = TEXT("Set Destination Node...");
		for (const TSubclassOf<UMDFastBindingDestinationBase>& DestinationClass : GetDestinationClasses())
		{
			OutAllActions.AddAction(MakeShared<FMDFastBindingSchemaAction_SetDestination>(DestinationClass), SetDestinationCategory);
		}
	}
}

bool SMDFastBindingEditorGraphWidget::CanDeleteSelectedNodes() const
{
	const FGraphPanelSelectionSet& SelectedNodes = GraphEditor->GetSelectedNodes();
	for (UObject* SelectedNode : SelectedNodes)
	{
		if (SelectedNode != nullptr && SelectedNode->IsA<UMDFastBindingGraphNode>())
		{
			return true;
		}
	}

	return false;
}

void SMDFastBindingEditorGraphWidget::DeleteSelectedNodes() const
{
	const FScopedTransaction Transaction(FGenericCommands::Get().Delete->GetDescription());

	const TSet<UObject*>& SelectedNodeSet = GraphEditor->GetSelectedNodes();

	TSet<UObject*> ValuesBeingDeleted;
	Algo::Transform(SelectedNodeSet, ValuesBeingDeleted, [](UObject* InNode) -> UMDFastBindingObject*
	{
		if (const UMDFastBindingGraphNode* GraphNode = Cast<UMDFastBindingGraphNode>(InNode))
		{
			return GraphNode->GetBindingObject();
		}

		return nullptr;
	});
	
	for (UObject* SelectedNode : SelectedNodeSet)
	{
		if (UMDFastBindingGraphNode* GraphNode = Cast<UMDFastBindingGraphNode>(SelectedNode))
		{
			GraphNode->DeleteNode(ValuesBeingDeleted);
		}
	}

	GraphEditor->ClearSelectionSet();
	RefreshGraph();
}

void SMDFastBindingEditorGraphWidget::RenameSelectedNode() const
{
	const FGraphPanelSelectionSet& SelectedNodes = GraphEditor->GetSelectedNodes();
	for (UObject* SelectedNode : SelectedNodes)
	{
		if (const UMDFastBindingGraphNode* GraphNode = Cast<UMDFastBindingGraphNode>(SelectedNode))
		{
			GraphEditor->JumpToNode(GraphNode, true);
			break;
		}
	}
}

void SMDFastBindingEditorGraphWidget::CopySelectedNodes() const
{
	const FGraphPanelSelectionSet& SelectedNodes = GraphEditor->GetSelectedNodes();
	TSet<UObject*> NodesToCopy;

	for (UObject* NodeObject : SelectedNodes)
	{
		if (UMDFastBindingGraphNode* BindingNode = Cast<UMDFastBindingGraphNode>(NodeObject))
		{
			BindingNode->PrepareForCopying();
			NodesToCopy.Add(BindingNode);
		}
	}

	if(NodesToCopy.Num() > 0)
	{
		FString ExportedText;
		FEdGraphUtilities::ExportNodesToText(NodesToCopy, /*out*/ ExportedText);
		FPlatformApplicationMisc::ClipboardCopy(*ExportedText);
	}

	for (UObject* NodeObject : NodesToCopy)
	{
		if (UMDFastBindingGraphNode* BindingNode = Cast<UMDFastBindingGraphNode>(NodeObject))
		{
			BindingNode->CleanUpCopying();
		}
	}
}

void SMDFastBindingEditorGraphWidget::PasteNodes() const
{
	GraphEditor->ClearSelectionSet();

	// Grab the text to paste from the clipboard.
	FString TextToImport;
	FPlatformApplicationMisc::ClipboardPaste(TextToImport);

	// Create temporary graph
	const FName UniqueGraphName = MakeUniqueObjectName(GetTransientPackage(), UWorld::StaticClass(), FName(*(LOCTEXT("MDFastBindingTempGraph", "TempGraph").ToString())));
	const TStrongObjectPtr<UMDFastBindingGraph> TempBindingGraph = TStrongObjectPtr<UMDFastBindingGraph>(NewObject<UMDFastBindingGraph>(GetTransientPackage(), UniqueGraphName));
	TempBindingGraph->Schema = UMDFastBindingGraphSchema::StaticClass();

	TSet<UEdGraphNode*> PastedNodes;
	FEdGraphUtilities::ImportNodesFromText(TempBindingGraph.Get(), TextToImport, /*out*/ PastedNodes);

	TArray<UMDFastBindingObject*> BindingObjects;
	for(UEdGraphNode* Node : PastedNodes)
	{
		UMDFastBindingGraphNode* BindingNode = Cast<UMDFastBindingGraphNode>(Node);
		if (BindingNode && BindingNode->CanDuplicateNode())
		{
			BindingObjects.Add(BindingNode->GetCopiedBindingObject());
		}
	}
	FVector2D AvgNodePosition(0.0f, 0.0f);

	for (const UMDFastBindingObject* Object : BindingObjects)
	{
		AvgNodePosition.X += Object->NodePos.X;
		AvgNodePosition.Y += Object->NodePos.Y;
	}

	const float InvNumNodes = 1.0f / float(PastedNodes.Num());
	AvgNodePosition.X *= InvNumNodes;
	AvgNodePosition.Y *= InvNumNodes;

	for (UMDFastBindingObject* Object : BindingObjects)
	{
		Object->NodePos.X = (Object->NodePos.X - AvgNodePosition.X);
		Object->NodePos.Y = (Object->NodePos.Y - AvgNodePosition.Y);
	}

	TArray<UMDFastBindingObject*> NewObjects;
	if(BindingObjects.Num() > 0)
	{
		FScopedTransaction Transaction(FGenericCommands::Get().Paste->GetDescription());

		if(UMDFastBindingInstance* BindingPtr = Binding.Get())
		{
			for (UMDFastBindingObject* BindingObject : BindingObjects)
			{
				if (UMDFastBindingDestinationBase* BindingDest = Cast<UMDFastBindingDestinationBase>(BindingObject))
				{
					NewObjects.Add(BindingPtr->SetDestination(BindingDest));
				}
				else if (UMDFastBindingValueBase* BindingValue = Cast<UMDFastBindingValueBase>(BindingObject))
				{
					NewObjects.Add(BindingPtr->AddOrphan(BindingValue));
				}
			}
		}
	}

	RefreshGraph();
	
	for (UMDFastBindingObject* Object : NewObjects)
	{
		if (UMDFastBindingGraphNode* Node = GraphObj->FindNodeWithBindingObject(Object))
		{
			GraphEditor->SetNodeSelection(Node, true);
		}
	}
}

bool SMDFastBindingEditorGraphWidget::CanPasteNodes() const
{
	FString ClipboardContent;
	FPlatformApplicationMisc::ClipboardPaste(ClipboardContent);

	return FEdGraphUtilities::CanImportNodesFromText(GraphObj, ClipboardContent);
}

void SMDFastBindingEditorGraphWidget::CutSelectedNodes() const
{
	CopySelectedNodes();
	DeleteSelectedNodes();
}

bool SMDFastBindingEditorGraphWidget::CanCutSelectedNodes() const
{
	return CanDeleteSelectedNodes();
}

bool SMDFastBindingEditorGraphWidget::CanSetDestinationActive() const
{
	const FGraphPanelSelectionSet& SelectedNodes = GraphEditor->GetSelectedNodes();
	if (SelectedNodes.Num() != 1)
	{
		return false;
	}
	
	if (const UMDFastBindingGraphNode* GraphNode = Cast<UMDFastBindingGraphNode>(SelectedNodes.Array()[0]))
	{
		if (const UMDFastBindingDestinationBase* BindingDest = Cast<UMDFastBindingDestinationBase>(GraphNode->GetBindingObject()))
		{
			return !BindingDest->IsActive();
		}
	}

	return false;
}

void SMDFastBindingEditorGraphWidget::SetDestinationActive() const
{
	const FGraphPanelSelectionSet& SelectedNodes = GraphEditor->GetSelectedNodes();
	if (SelectedNodes.Num() != 1)
	{
		return;
	}
	
	if (const UMDFastBindingGraphNode* GraphNode = Cast<UMDFastBindingGraphNode>(SelectedNodes.Array()[0]))
	{
		if (UMDFastBindingDestinationBase* BindingDest = Cast<UMDFastBindingDestinationBase>(GraphNode->GetBindingObject()))
		{
			if (UMDFastBindingInstance* BindingPtr = Binding.Get())
			{
				BindingPtr->SetDestination(BindingDest);
				RefreshGraph();
			}
		}
	}
}

void SMDFastBindingEditorGraphWidget::RemoveExtendablePin(TWeakObjectPtr<UMDFastBindingObject> BindingObject, int32 ItemIndex) const
{
	if (UMDFastBindingObject* Object = BindingObject.Get())
	{
		FScopedTransaction Transaction(LOCTEXT("DeletePinTransactionName", "Delete Pin"));
		Object->Modify();
		Object->RemoveExtendablePinBindingItem(ItemIndex);
		RefreshGraph();
	}
}

#undef LOCTEXT_NAMESPACE
