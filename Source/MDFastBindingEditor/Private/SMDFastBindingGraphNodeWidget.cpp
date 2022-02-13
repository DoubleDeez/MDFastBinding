#include "SMDFastBindingGraphNodeWidget.h"

#include "MDFastBindingGraphNode.h"
#include "MDFastBindingObject.h"
#include "NodeFactory.h"

#define LOCTEXT_NAMESPACE "MDFastBindingGraphNodeWidget"

void SMDFastBindingGraphNodeWidget::Construct(const FArguments& InArgs, UMDFastBindingGraphNode* InNode)
{
	GraphNode = InNode;
	UpdateGraphNode();
}

#if ENGINE_MAJOR_VERSION <= 4
void SMDFastBindingGraphNodeWidget::MoveTo(const FVector2D& NewPosition, FNodeSet& NodeFilter, bool bMarkDirty)
#else
void SMDFastBindingGraphNodeWidget::MoveTo(const FVector2D& NewPosition, FNodeSet& NodeFilter)
#endif
{
#if ENGINE_MAJOR_VERSION <= 4
	SGraphNode::MoveTo(NewPosition, NodeFilter, bMarkDirty);
#else
	SGraphNode::MoveTo(NewPosition, NodeFilter);
#endif

	if (UMDFastBindingGraphNode* Node = GetGraphNode())
	{
		Node->OnMoved();
	}
}

UMDFastBindingGraphNode* SMDFastBindingGraphNodeWidget::GetGraphNode() const
{
	return Cast<UMDFastBindingGraphNode>(GraphNode);
}

void SMDFastBindingGraphNodeWidget::UpdateErrorInfo()
{
	ErrorColor = FLinearColor(0,0,0);
	ErrorMsg.Empty();
	
	if (const UMDFastBindingGraphNode* Node = GetGraphNode())
	{
		if (UMDFastBindingObject* BindingObject = Node->GetBindingObject())
		{
			TArray<FText> Errors;
			if (BindingObject->IsDataValid(Errors) == EDataValidationResult::Invalid)
			{
				ErrorColor = FEditorStyle::GetColor("ErrorReporting.BackgroundColor");
				
				if (Errors.Num() == 0)
				{
					ErrorMsg = LOCTEXT("InvalidWithoutErrors", "Unknown Error").ToString();
				}
				else
				{
					ErrorMsg = Errors.Last().ToString();
				}
			}
		}
	}
}

TSharedPtr<SGraphPin> SMDFastBindingGraphNodeWidget::CreatePinWidget(UEdGraphPin* Pin) const
{
	if (IsSelfPin(*Pin))
	{
		return SNew(SMDFastBindingSelfGraphPinWidget, Pin);
	}
	else if (TSharedPtr<SGraphPin> K2Pin = FNodeFactory::CreateK2PinWidget(Pin))
	{
		return K2Pin;
	}
	
	return SGraphNode::CreatePinWidget(Pin);
}

bool SMDFastBindingGraphNodeWidget::IsSelfPin(UEdGraphPin& Pin) const
{
	if (const UMDFastBindingGraphNode* Node = GetGraphNode())
	{
		if (const UMDFastBindingObject* BindingObject = Node->GetBindingObject())
		{
			return BindingObject->DoesBindingItemDefaultToSelf(Pin.GetFName());
		}
	}

	return false;
}

#undef LOCTEXT_NAMESPACE
