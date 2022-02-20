#include "MDFastBindingGraphNode.h"

#include "MDFastBindingEditorStyle.h"
#include "MDFastBindingGraph.h"
#include "MDFastBindingInstance.h"
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

void UMDFastBindingGraphNode::DeleteNode(const TSet<UObject*>& OrphanExclusionSet)
{
	if (UMDFastBindingObject* Object = BindingObject.Get())
	{
		if (UMDFastBindingDestinationBase* BindingDest = Cast<UMDFastBindingDestinationBase>(Object))
		{
			if (UMDFastBindingInstance* Binding = BindingDest->GetOuterBinding())
			{
				Binding->Modify();
				Binding->RemoveDestination(BindingDest);
				BindingDest->OrphanAllBindingItems(OrphanExclusionSet);
			}
			
			return;
		}
		
		UEdGraphPin* OutputPin = FindPin(OutputPinName);
		if (OutputPin == nullptr || OutputPin->LinkedTo.Num() == 0)
		{
			// We're probably an orphan
			if (UMDFastBindingValueBase* ValueBase = Cast<UMDFastBindingValueBase>(Object))
			{
				if (UMDFastBindingInstance* Binding = ValueBase->GetOuterBinding())
				{
					Binding->Modify();
					Binding->RemoveOrphan(ValueBase);
					ValueBase->OrphanAllBindingItems(OrphanExclusionSet);
				}
			}
			
			return;
		}

		UEdGraphPin* ConnectedPin = OutputPin->LinkedTo[0];
		if (ConnectedPin == nullptr)
		{
			return;
		}

		if (UMDFastBindingValueBase* BindingValue = Cast<UMDFastBindingValueBase>(Object))
		{
			BindingValue->Modify();
			BindingValue->OrphanAllBindingItems(OrphanExclusionSet);
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

void UMDFastBindingGraphNode::PrepareForCopying()
{
	Super::PrepareForCopying();

	if (const UMDFastBindingObject* Obj = GetBindingObject())
	{
		// Don't want to copy on sub-objects
		CopiedObject = DuplicateObject<UMDFastBindingObject>(Obj, this);
		CopiedObject->ClearBindingItemValuePtrs();
	}
}

void UMDFastBindingGraphNode::CleanUpCopying()
{
	CopiedObject = nullptr;
}

FText UMDFastBindingGraphNode::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (UMDFastBindingDestinationBase* BindingDest = Cast<UMDFastBindingDestinationBase>(GetBindingObject()))
	{
		if (!BindingDest->IsActive())
		{
			static const FText InactiveFormat = LOCTEXT("InactiveDestinationNodeTitleFormat", "{0} (Inactive)");
			return FText::Format(InactiveFormat, BindingDest->GetDisplayName());
		}
	}
	
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
	if (const UMDFastBindingDestinationBase* BindingDest = Cast<UMDFastBindingDestinationBase>(GetBindingObject()))
	{
		if (BindingDest->IsActive())
		{
			return FMDFastBindingEditorStyle::Get().GetColor(TEXT("DestinationNodeTitleColor"));
		}

		return FMDFastBindingEditorStyle::Get().GetColor(TEXT("InactiveDestinationNodeTitleColor"));
	}

	return FMDFastBindingEditorStyle::Get().GetColor(TEXT("NodeTitleColor"));
}

FText UMDFastBindingGraphNode::GetTooltipText() const
{
	if (UMDFastBindingDestinationBase* BindingDest = Cast<UMDFastBindingDestinationBase>(GetBindingObject()))
	{
		if (!BindingDest->IsActive())
		{
			static const FText InactiveToolTipFormat = LOCTEXT("InactiveDestinationNodeToolTipFormat", "This binding destination is inactive and will not run. Right-click this node to set it active. Only one destination can be active within a binding at a time.\n{0}");
			return FText::Format(InactiveToolTipFormat, BindingDest->GetToolTipText());
		}
	}
	
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
		
		for (FMDFastBindingItem& Item : Object->GetBindingItems())
		{
			if (const FProperty* ItemProp = Item.ItemProperty.Get())
			{
				FEdGraphPinType ItemPinType;
				const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
				K2Schema->ConvertPropertyToPinType(ItemProp, ItemPinType);
				if (UEdGraphPin* Pin = CreatePin(EGPD_Input, ItemPinType, Item.ItemName))
				{
					Pin->PinToolTip = Item.ToolTip.ToString();
					
					if (Item.HasValue())
					{
						Pin->DefaultObject = Item.DefaultObject;
						Pin->DefaultValue = Item.DefaultString;
						Pin->DefaultTextValue = Item.DefaultText;
					}
					else
					{
						K2Schema->SetPinAutogeneratedDefaultValueBasedOnType(Pin);
						Object->Modify();
						Item.DefaultObject = Pin->DefaultObject;
						Item.DefaultString = Pin->DefaultValue;
						Item.DefaultText = Pin->DefaultTextValue;
					}
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
