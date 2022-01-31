#include "SMDFastBindingEditorGraphWidget.h"

#include "MDFastBindingGraph.h"
#include "MDFastBindingGraphNode.h"
#include "MDFastBindingGraphSchema.h"
#include "MDFastBindingObject.h"
#include "SGraphActionMenu.h"
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

	GraphEditorCommands = MakeShared<FUICommandList>();

	GraphEditorCommands->MapAction(FGenericCommands::Get().Delete,
		FExecuteAction::CreateSP(this, &SMDFastBindingEditorGraphWidget::DeleteSelectedNodes),
		FCanExecuteAction::CreateSP(this, &SMDFastBindingEditorGraphWidget::CanDeleteSelectedNodes)
		);

	GraphEditorCommands->MapAction(FGenericCommands::Get().Rename,
		FExecuteAction::CreateSP(this, &SMDFastBindingEditorGraphWidget::RenameSelectedNode),
		FCanExecuteAction()
		);
}

void SMDFastBindingEditorGraphWidget::SetBinding(UMDFastBindingDestinationBase* InBinding)
{
	Binding = InBinding;
	GraphObj->SetBindingDestination(InBinding);
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

FActionMenuContent SMDFastBindingEditorGraphWidget::OnCreateActionMenu(UEdGraph* InGraph, const FVector2D& InNodePosition, const TArray<UEdGraphPin*>& InDraggedPins, bool bAutoExpand, SGraphEditor::FActionMenuClosed InOnMenuClose)
{
	const TSharedRef<SGraphActionMenu> ActionMenu = SNew(SGraphActionMenu)
						.OnActionSelected(this, &SMDFastBindingEditorGraphWidget::OnActionSelected, InNodePosition, InDraggedPins)
						.OnCollectAllActions(this, &SMDFastBindingEditorGraphWidget::CollectAllActions, InDraggedPins)
						.DraggedFromPins(InDraggedPins)
						.GraphObj(GraphObj);
	
	return FActionMenuContent(
		SNew(SBorder)
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
		
		UMDFastBindingObject* BindingObject = GraphNode->GetBindingObject();
		if (BindingObject != nullptr && !BindingObject->IsA<UMDFastBindingDestinationBase>())
		{
			MenuBuilder->AddMenuEntry(FGenericCommands::Get().Delete);
		}
	}
    	
	return FActionMenuContent(MenuBuilder->MakeWidget());
}

void SMDFastBindingEditorGraphWidget::OnActionSelected(const TArray<TSharedPtr<FEdGraphSchemaAction>>& SelectedActions, ESelectInfo::Type InSelectionType, FVector2D InNodePosition, TArray<UEdGraphPin*> InDraggedPins) const
{
	if (InDraggedPins.Num() > 0 && (InSelectionType == ESelectInfo::OnMouseClick  || InSelectionType == ESelectInfo::OnKeyPress))
	{
		for (TSharedPtr<FEdGraphSchemaAction> SelectedAction : SelectedActions)
		{
			if (SelectedAction.IsValid() && GraphObj != nullptr)
			{
				SelectedAction->PerformAction(GraphObj, InDraggedPins[0], InNodePosition);
			}
		}
	}

	FSlateApplication::Get().DismissAllMenus();
}

void SMDFastBindingEditorGraphWidget::CollectAllActions(FGraphActionListBuilderBase& OutAllActions, TArray<UEdGraphPin*> InDraggedPins)
{
	if (InDraggedPins.Num() == 1 && InDraggedPins[0] != nullptr)
	{
		const UEdGraphPin* Pin = InDraggedPins[0];
		if (Pin->Direction == EGPD_Input)
		{
			static const FString CreateValueCategory = TEXT("Create Value Node...");
			for (const TSubclassOf<UMDFastBindingValueBase>& ValueClass : GetValueClasses())
			{
				OutAllActions.AddAction(MakeShared<FMDFastBindingSchemaAction_CreateValue>(ValueClass), CreateValueCategory);
			}
		}
	}
}

bool SMDFastBindingEditorGraphWidget::CanDeleteSelectedNodes() const
{
	const FGraphPanelSelectionSet& SelectedNodes = GraphEditor->GetSelectedNodes();
	for (UObject* SelectedNode : SelectedNodes)
	{
		if (const UMDFastBindingGraphNode* GraphNode = Cast<UMDFastBindingGraphNode>(SelectedNode))
		{
			// Can't delete the destination node, return true if we have anything else selected
			UMDFastBindingObject* BindingObject = GraphNode->GetBindingObject();
			if (BindingObject != nullptr && !BindingObject->IsA<UMDFastBindingDestinationBase>())
			{
				return true;
			}
		}
	}

	return false;
}

void SMDFastBindingEditorGraphWidget::DeleteSelectedNodes() const
{
	const FScopedTransaction Transaction(FGenericCommands::Get().Delete->GetDescription());

	const FGraphPanelSelectionSet& SelectedNodes = GraphEditor->GetSelectedNodes();
	for (UObject* SelectedNode : SelectedNodes)
	{
		if (UMDFastBindingGraphNode* GraphNode = Cast<UMDFastBindingGraphNode>(SelectedNode))
		{
			// TODO - orphan child items
			GraphNode->DeleteNode();
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

#undef LOCTEXT_NAMESPACE
