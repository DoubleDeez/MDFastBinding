#include "SMDFastBindingEditorGraphWidget.h"

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

#define LOCTEXT_NAMESPACE "SMDFastBindingEditorGraphWidget"

SMDFastBindingEditorGraphWidget::~SMDFastBindingEditorGraphWidget()
{
	GraphObj->RemoveFromRoot();
}

void SMDFastBindingEditorGraphWidget::Construct(const FArguments& InArgs)
{
	GraphObj = NewObject<UMDFastBindingGraph>();
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
		.BorderImage(FEditorStyle::GetBrush("Menu.Background"))
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
		
		UMDFastBindingObject* BindingObject = GraphNode->GetBindingObject();
		if (const UMDFastBindingDestinationBase* BindingDest = Cast<UMDFastBindingDestinationBase>(BindingObject))
		{
			if (!BindingDest->IsActive())
			{
				MenuBuilder->AddMenuEntry(FMDFastBindingEditorCommands::Get().SetDestinationActive);
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
	if (InDraggedPins.Num() == 0 || (InDraggedPins.Num() == 1 && InDraggedPins[0] != nullptr && InDraggedPins[0]->Direction == EGPD_Input))
	{
		static const FString CreateValueCategory = TEXT("Create Value Node...");
		for (const TSubclassOf<UMDFastBindingValueBase>& ValueClass : GetValueClasses())
		{
			OutAllActions.AddAction(MakeShared<FMDFastBindingSchemaAction_CreateValue>(ValueClass), CreateValueCategory);
		}
	}

	if (InDraggedPins.Num() == 0)
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

#undef LOCTEXT_NAMESPACE
