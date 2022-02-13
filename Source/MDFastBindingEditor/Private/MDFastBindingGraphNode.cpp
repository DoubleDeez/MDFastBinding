#include "MDFastBindingGraphNode.h"

#include "MDFastBindingEditorStyle.h"
#include "MDFastBindingGraph.h"
#include "MDFastBindingObject.h"
#include "SMDFastBindingGraphNodeWidget.h"
#include "BindingDestinations/MDFastBindingDestinationBase.h"
#include "BindingValues/MDFastBindingValueBase.h"

#define LOCTEXT_NAMESPACE "MDFastBindingGraphNode"

const FName UMDFastBindingGraphNode::OutputPinName = TEXT("Output");

UMDFastBindingGraphNode::UMDFastBindingGraphNode()
{
	bCanRenameNode = true;
}

void UMDFastBindingGraphNode::SetBindingObject(UMDFastBindingObject* InObject)
{
	BindingObject = InObject;
	if (InObject != nullptr)
	{
		NodePosX = InObject->NodePos.X;
		NodePosY = InObject->NodePos.Y;
		NodeComment = InObject->DevComment;
		bCommentBubbleVisible = InObject->bIsCommentBubbleVisible;
	}
}

void UMDFastBindingGraphNode::ClearConnection(const FName& PinName)
{
	if (UMDFastBindingObject* Object = BindingObject.Get())
	{
		Object->Modify();
		Object->ClearBindingItemValue(PinName);
	}
}

void UMDFastBindingGraphNode::OnMoved()
{
	if (UMDFastBindingObject* Object = BindingObject.Get())
	{
		Object->Modify();
		Object->NodePos = FIntPoint(NodePosX, NodePosY);
	}
}

void UMDFastBindingGraphNode::DeleteNode()
{
	if (UMDFastBindingObject* Object = BindingObject.Get())
	{
		if (Object->IsA<UMDFastBindingDestinationBase>())
		{
			// Can't delete destination
			return;
		}
		
		UEdGraphPin* OutputPin = FindPin(OutputPinName);
		if (OutputPin == nullptr || OutputPin->LinkedTo.Num() == 0)
		{
			return;
		}

		UEdGraphPin* ConnectedPin = OutputPin->LinkedTo[0];
		if (ConnectedPin == nullptr)
		{
			return;
		}

		UMDFastBindingGraphNode* ConnectedNode = Cast<UMDFastBindingGraphNode>(ConnectedPin->GetOwningNode());
		if (ConnectedNode == nullptr)
		{
			return;
		}

		ConnectedNode->ClearConnection(ConnectedPin->GetFName());
	}
}

void UMDFastBindingGraphNode::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	if (UMDFastBindingObject* Object = BindingObject.Get())
	{
		if (FMDFastBindingItem* Item = Object->FindBindingItem(Pin->GetFName()))
		{
			Object->Modify();
			Item->DefaultObject = Pin->DefaultObject;
			Item->DefaultString = Pin->DefaultValue;
			Item->DefaultText = Pin->DefaultTextValue;
			RefreshGraph();
		}
	}
}

FText UMDFastBindingGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (UMDFastBindingObject* Object = BindingObject.Get())
	{
		return Object->GetDisplayName();
	}
	
	return Super::GetNodeTitle(TitleType);
}

TSharedPtr<SGraphNode> UMDFastBindingGraphNode::CreateVisualWidget()
{
	return SNew(SMDFastBindingGraphNodeWidget, this);
}

void UMDFastBindingGraphNode::OnRenameNode(const FString& NewName)
{
	if (UMDFastBindingObject* Object = BindingObject.Get())
	{
		Object->Modify();
		Object->DevName = FText::FromString(NewName);
	}
}

void UMDFastBindingGraphNode::OnUpdateCommentText(const FString& NewComment)
{
	if(!NodeComment.Equals(NewComment))
	{
		Modify();
		NodeComment	= NewComment;
		
		if (UMDFastBindingObject* Object = BindingObject.Get())
		{
			Object->Modify();
			Object->DevComment = NewComment;
		}
	}
}

void UMDFastBindingGraphNode::OnCommentBubbleToggled(bool bInCommentBubbleVisible)
{
	if (UMDFastBindingObject* Object = BindingObject.Get())
	{
		if (Object->bIsCommentBubbleVisible != bInCommentBubbleVisible)
		{
			Object->Modify();
			Object->bIsCommentBubbleVisible = bInCommentBubbleVisible;
		}
	}
}

FLinearColor UMDFastBindingGraphNode::GetNodeTitleColor() const
{
	if (Cast<UMDFastBindingDestinationBase>(GetBindingObject()) != nullptr)
	{
		return FMDFastBindingEditorStyle::Get().GetColor(TEXT("DestinationNodeTitleColor"));
	}

	return FMDFastBindingEditorStyle::Get().GetColor(TEXT("NodeTitleColor"));
}

FText UMDFastBindingGraphNode::GetTooltipText() const
{
	if (UMDFastBindingObject* Object = BindingObject.Get())
	{
		return Object->GetToolTipText();
	}
	
	return Super::GetTooltipText();
}

void UMDFastBindingGraphNode::RefreshGraph()
{
	if (UMDFastBindingGraph* MDGraph = Cast<UMDFastBindingGraph>(GetGraph()))
	{
		MDGraph->RefreshGraph();
	}
}

void UMDFastBindingGraphNode::AllocateDefaultPins()
{
	if (UMDFastBindingObject* Object = BindingObject.Get())
	{
		Object->SetupBindingItems();
		
		for (const FMDFastBindingItem& Item : Object->GetBindingItems())
		{
			if (const FProperty* ItemProp = Item.ItemProperty.Get())
			{
				FEdGraphPinType ItemPinType;
				GetDefault<UEdGraphSchema_K2>()->ConvertPropertyToPinType(ItemProp, ItemPinType);
				if (UEdGraphPin* Pin = CreatePin(EGPD_Input, ItemPinType, Item.ItemName))
				{
					Pin->PinToolTip = Item.ToolTip.ToString();
					Pin->DefaultObject = Item.DefaultObject;
					Pin->DefaultValue = Item.DefaultString;
					Pin->DefaultTextValue = Item.DefaultText;
				}
			}
		}

		if (UMDFastBindingValueBase* ValueObject = Cast<UMDFastBindingValueBase>(Object))
		{
			if (const FProperty* OutputProp = ValueObject->GetOutputProperty())
			{
				FEdGraphPinType OutputPinType;
				GetDefault<UEdGraphSchema_K2>()->ConvertPropertyToPinType(OutputProp, OutputPinType);
				CreatePin(EGPD_Output, OutputPinType, OutputPinName);
			}
			else
			{
				// Create an invalid pin so we show _something_
				CreatePin(EGPD_Output, NAME_None, OutputPinName);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
