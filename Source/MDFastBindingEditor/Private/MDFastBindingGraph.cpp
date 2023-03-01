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

	const TArray<UMDFastBindingObject*> Objects = Binding->GatherAllBindingObjects();

	for (UMDFastBindingObject* Object : Objects)
	{
		UMDFastBindingGraphNode* NewNode = Cast<UMDFastBindingGraphNode>(CreateNode(UMDFastBindingGraphNode::StaticClass(), false));
		NewNode->CreateNewGuid();
		NewNode->SetBindingObject(Object);
		NewNode->AllocateDefaultPins();
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

void UMDFastBindingGraph::SetBindingBeingDebugged(UMDFastBindingInstance* InBinding)
{
	BindingBeingDebugged = InBinding;
	for (UEdGraphNode* Node : Nodes)
	{
		if (UMDFastBindingGraphNode* BindingNode = Cast<UMDFastBindingGraphNode>(Node))
		{
			BindingNode->SetBindingBeingDebugged(InBinding);
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
