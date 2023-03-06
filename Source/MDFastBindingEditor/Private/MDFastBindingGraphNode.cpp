#include "MDFastBindingGraphNode.h"

#include "BindingDestinations/MDFastBindingDestinationBase.h"
#include "BindingValues/MDFastBindingValueBase.h"
#include "EdGraphSchema_K2.h"
#include "MDFastBindingEditorStyle.h"
#include "MDFastBindingGraph.h"
#include "MDFastBindingInstance.h"
#include "MDFastBindingObject.h"
#include "SMDFastBindingGraphNodeWidget.h"
#include "BindingDestinations/MDFastBindingDestination_Function.h"
#include "BindingValues/MDFastBindingValue_Function.h"

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

void UMDFastBindingGraphNode::SetBindingBeingDebugged(UMDFastBindingInstance* InBinding)
{
	BindingBeingDebugged = InBinding;
}

UMDFastBindingObject* UMDFastBindingGraphNode::GetBindingObjectBeingDebugged() const
{
	if (BindingObject.IsValid() && BindingBeingDebugged.IsValid())
	{
		return BindingBeingDebugged->FindBindingObjectWithGUID(BindingObject->BindingObjectIdentifier);
	}

	return nullptr;
}

UMDFastBindingGraphNode* UMDFastBindingGraphNode::GetLinkedOutputNode() const
{
	const UEdGraphPin* OutputPin = FindPin(OutputPinName);
	if (OutputPin == nullptr || OutputPin->LinkedTo.Num() == 0)
	{
		return nullptr;
	}

	return Cast<UMDFastBindingGraphNode>(OutputPin->LinkedTo[0]->GetOwningNode());
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
		CopiedObject = DuplicateObject<UMDFastBindingObject>(Obj, this);
	}
}

void UMDFastBindingGraphNode::CleanUpCopying()
{
	CopiedObject = nullptr;
}

void UMDFastBindingGraphNode::CleanUpPasting()
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
			return FText::Format(InactiveFormat, BindingDest->DevName.IsEmptyOrWhitespace() ? BindingDest->GetClass()->GetDisplayNameText() : BindingDest->DevName);
		}
	}
	
	if (UMDFastBindingObject* Object = BindingObject.Get())
	{
		return Object->DevName.IsEmptyOrWhitespace() ? Object->GetClass()->GetDisplayNameText() : Object->DevName;
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
		Object->SetupBindingItems_Internal();
		
		UFunction* Function = nullptr;
		if (UMDFastBindingDestination_Function* DestFunc = Cast<UMDFastBindingDestination_Function>(BindingObject))
		{
			Function = DestFunc->GetFunction();
		}
		else if (UMDFastBindingValue_Function* ValueFunc = Cast<UMDFastBindingValue_Function>(BindingObject))
		{
			Function = ValueFunc->GetFunction();
		}
		
		for (FMDFastBindingItem& Item : Object->GetBindingItems())
		{
			UEdGraphPin* Pin = nullptr;
			const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();
			const FProperty* ItemProp = Item.ResolveOutputProperty();
			if (ItemProp != nullptr)
			{
				FEdGraphPinType ItemPinType;
				K2Schema->ConvertPropertyToPinType(ItemProp, ItemPinType);
				Pin = CreatePin(EGPD_Input, ItemPinType, Item.ItemName);
			}
			else
			{
				Pin = CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Wildcard, Item.ItemName);
			}
			
			if (Pin != nullptr)
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
					// Special cases for color structs
					UScriptStruct* LinearColorStruct = TBaseStructure<FLinearColor>::Get();
					UScriptStruct* ColorStruct = TBaseStructure<FColor>::Get();
					
					FString ParamValue;
					if (Function != nullptr && K2Schema->FindFunctionParameterDefaultValue(Function, ItemProp, ParamValue))
					{
						K2Schema->SetPinAutogeneratedDefaultValue(Pin, ParamValue);
					}
					else if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct && Pin->PinType.PinSubCategoryObject == ColorStruct)
					{
						K2Schema->SetPinAutogeneratedDefaultValue(Pin, FColor::White.ToString());
					}
					else if (Pin->PinType.PinCategory == UEdGraphSchema_K2::PC_Struct && Pin->PinType.PinSubCategoryObject == LinearColorStruct)
					{
						K2Schema->SetPinAutogeneratedDefaultValue(Pin, FLinearColor::White.ToString());
					}
					else
					{
						K2Schema->SetPinAutogeneratedDefaultValueBasedOnType(Pin);
					}
					
					const bool bDidDefaultsChange = Item.DefaultObject != Pin->DefaultObject
						|| Item.DefaultString != Pin->DefaultValue
						|| !Item.DefaultText.EqualTo(Pin->DefaultTextValue);
					if (bDidDefaultsChange)
					{
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
