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
void SMDFastBindingGraphNodeWidget::MoveTo(const FVector2D& NewPosition, FNodeSet& NodeFilter)
#else
void SMDFastBindingGraphNodeWidget::MoveTo(const FVector2D& NewPosition, FNodeSet& NodeFilter, bool bMarkDirty)
#endif
{
#if ENGINE_MAJOR_VERSION <= 4
	SGraphNode::MoveTo(NewPosition, NodeFilter);
#else
	SGraphNode::MoveTo(NewPosition, NodeFilter, bMarkDirty);
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

UMDFastBindingObject* SMDFastBindingGraphNodeWidget::GetBindingObject() const
{
	if (const UMDFastBindingGraphNode* Node = GetGraphNode())
	{
		return Node->GetBindingObject();
	}

	return nullptr;
}

void SMDFastBindingGraphNodeWidget::UpdateErrorInfo()
{
	ErrorColor = FLinearColor(0,0,0);
	ErrorMsg.Empty();
	
	if (UMDFastBindingObject* BindingObject = GetBindingObject())
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

void SMDFastBindingGraphNodeWidget::CreateBelowPinControls(TSharedPtr<SVerticalBox> MainBox)
{
	if (UMDFastBindingObject* BindingObject = GetBindingObject())
	{
		// HACK - this function implies we should add below the pins, but inserting at Slot 1 puts this widget right below the title bar
		MainBox->InsertSlot(1)
		.AutoHeight()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Center)
		.Padding(10.f, 2.f)
		[
			BindingObject->CreateNodeHeaderWidget()
		];
	}
}

bool SMDFastBindingGraphNodeWidget::IsSelfPin(UEdGraphPin& Pin) const
{
	if (const UMDFastBindingObject* BindingObject = GetBindingObject())
	{
		return BindingObject->DoesBindingItemDefaultToSelf(Pin.GetFName());
	}

	return false;
}

#undef LOCTEXT_NAMESPACE
