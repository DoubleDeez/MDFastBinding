#include "SMDFastBindingGraphNodeWidget.h"

#include "MDFastBindingGraphNode.h"
#include "MDFastBindingObject.h"

#define LOCTEXT_NAMESPACE "MDFastBindingGraphNodeWidget"

void SMDFastBindingGraphNodeWidget::Construct(const FArguments& InArgs, UMDFastBindingGraphNode* InNode)
{
	GraphNode = InNode;
	UpdateGraphNode();
}

void SMDFastBindingGraphNodeWidget::MoveTo(const FVector2D& NewPosition, FNodeSet& NodeFilter)
{
	SGraphNode::MoveTo(NewPosition, NodeFilter);

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

#undef LOCTEXT_NAMESPACE
