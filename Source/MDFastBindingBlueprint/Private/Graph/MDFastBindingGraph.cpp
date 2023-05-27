#include "Graph/MDFastBindingGraph.h"

#include "BindingValues/MDFastBindingValueBase.h"
#include "EdGraph/EdGraphPin.h"
#include "GraphEditAction.h"
#include "Graph/MDFastBindingGraphNode.h"
#include "Graph/SMDFastBindingEditorGraphWidget.h"
#include "MDFastBindingInstance.h"
#include "MDFastBindingObject.h"

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
	Nodes.Empty();
	Binding = InBinding;

	if (InBinding == nullptr)
	{
		return;
	}

	const TArray<UMDFastBindingObject*> Objects = Binding->GatherAllBindingObjects();
	if (Objects.IsEmpty())
	{
		NotifyGraphChanged(FEdGraphEditAction(EEdGraphActionType::GRAPHACTION_Default, this, nullptr, false));
		return;
	}

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
								// Don't use Pin->MakeLinkTo as that will mark our widget package dirty which we don't want
								if (!Pin->LinkedTo.Contains(ConnectedPin))
								{
									Pin->LinkedTo.Add(ConnectedPin);
									ConnectedPin->LinkedTo.Add(Pin);
								}
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
