#include "MDFastBindingGraph.h"

#include "MDFastBindingGraphNode.h"
#include "MDFastBindingInstance.h"
#include "MDFastBindingObject.h"
#include "SMDFastBindingEditorGraphWidget.h"
#include "BindingValues/MDFastBindingValueBase.h"
#include "BindingDestinations/MDFastBindingDestinationBase.h"

void UMDFastBindingGraph::SetGraphWidget(TSharedRef<SMDFastBindingEditorGraphWidget> InGraphWidget)
{
	GraphWidget = InGraphWidget;
}

void UMDFastBindingGraph::RefreshGraph()
{
	SetBinding(Binding.Get());
}

void UMDFastBindingGraph::SetBinding(UMDFastBindingInstance* InBinding)
{
	TArray<UEdGraphNode*> NodesCopy = Nodes;
	for (UEdGraphNode* Node : NodesCopy)
	{
		RemoveNode(Node);
	}

	Binding = InBinding;

	if (InBinding == nullptr)
	{
		return;
	}

	// Construct Nodes
	TArray<UMDFastBindingObject*> NextNodes;
	if (UMDFastBindingDestinationBase* BindingDest = InBinding->GetBindingDestination())
	{
		NextNodes.Add(BindingDest);		
	}
	NextNodes.Append(InBinding->OrphanedValues);
	NextNodes.Append(InBinding->InactiveDestinations);
	while (NextNodes.Num() > 0)
	{
		TArray<UMDFastBindingObject*> CurrentNodes = MoveTemp(NextNodes);
		NextNodes.Empty();

		while (CurrentNodes.Num() > 0)
		{
			UMDFastBindingObject* Node = CurrentNodes[0];
			CurrentNodes.RemoveAt(0);
			
			for (const FMDFastBindingItem& Item : Node->GetBindingItems())
			{
				if (Item.Value != nullptr)
				{
					NextNodes.Add(Item.Value);
				}
			}

			UMDFastBindingGraphNode* NewNode = Cast<UMDFastBindingGraphNode>(CreateNode(UMDFastBindingGraphNode::StaticClass(), false));
			NewNode->CreateNewGuid();
			NewNode->SetBindingObject(Node);
			NewNode->AllocateDefaultPins();
		}
	}

	// Form pin connections
	for (UEdGraphNode* Node : Nodes)
	{
		if (const UMDFastBindingGraphNode* BindingNode = Cast<UMDFastBindingGraphNode>(Node))
		{
			if (const UMDFastBindingObject* BindingObject = BindingNode->GetBindingObject())
			{
				for (UEdGraphPin* Pin : BindingNode->GetAllPins())
				{
					const FMDFastBindingItem* PinItem = BindingObject->FindBindingItem(Pin->PinName);
					if (Pin->Direction == EGPD_Input && PinItem != nullptr && PinItem->Value != nullptr)
					{
						if (const UMDFastBindingGraphNode* ConnectedNode = FindNodeWithBindingObject(PinItem->Value))
						{
							if (UEdGraphPin* ConnectedPin = ConnectedNode->FindPin(UMDFastBindingGraphNode::OutputPinName, EGPD_Output))
							{
								Pin->MakeLinkTo(ConnectedPin);
							}
						}
					}
				}
			}
		}
	}
}

UMDFastBindingGraphNode* UMDFastBindingGraph::FindNodeWithBindingObject(UMDFastBindingObject* InObject) const
{
	for (UEdGraphNode* Node : Nodes)
	{
		if (UMDFastBindingGraphNode* BindingNode = Cast<UMDFastBindingGraphNode>(Node))
		{
			if (BindingNode->GetBindingObject() == InObject)
			{
				return BindingNode;
			}
		}
	}

	return nullptr;
}

void UMDFastBindingGraph::SelectNodeWithBindingObject(UMDFastBindingObject* InObject)
{
	if (UMDFastBindingGraphNode* NodeWithObject = FindNodeWithBindingObject(InObject))
	{
		if (const TSharedPtr<SMDFastBindingEditorGraphWidget> GraphWidgetPtr = GraphWidget.Pin())
		{
			GraphWidgetPtr->SetSelection(NodeWithObject, true);
		}
	}
}

void UMDFastBindingGraph::ClearSelection()
{
	if (const TSharedPtr<SMDFastBindingEditorGraphWidget> GraphWidgetPtr = GraphWidget.Pin())
	{
		GraphWidgetPtr->ClearSelection();
	}
}
