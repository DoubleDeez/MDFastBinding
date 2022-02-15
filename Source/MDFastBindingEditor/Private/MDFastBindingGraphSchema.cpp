#include "MDFastBindingGraphSchema.h"

#include "BlueprintConnectionDrawingPolicy.h"
#include "MDFastBindingEditorStyle.h"
#include "MDFastBindingGraph.h"
#include "MDFastBindingGraphNode.h"
#include "BindingDestinations/MDFastBindingDestinationBase.h"
#include "BindingValues/MDFastBindingValueBase.h"

#define LOCTEXT_NAMESPACE "MDFastBindingGraphSchema"

FMDFastBindingSchemaAction_CreateValue::FMDFastBindingSchemaAction_CreateValue(TSubclassOf<UMDFastBindingValueBase> InValueClass)
		: ValueClass(InValueClass)
{
	UpdateSearchData(ValueClass->GetDisplayNameText()
		, ValueClass->GetToolTipText()
		, FText::GetEmpty()
		, FText::GetEmpty());
}

FName FMDFastBindingSchemaAction_CreateValue::GetTypeId() const
{
	return ValueClass->GetFName();
}

UEdGraphNode* FMDFastBindingSchemaAction_CreateValue::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	UMDFastBindingGraph* Graph = Cast<UMDFastBindingGraph>(ParentGraph);
	if (Graph == nullptr || ValueClass == nullptr)
	{
		return nullptr;
	}

	auto InitValueAndNode = [Graph, Location](UMDFastBindingValueBase* Value)
	{
		Value->NodePos = Location.IntPoint();
		Graph->ClearSelection();
		Graph->RefreshGraph();
		Graph->SelectNodeWithBindingObject(Value);
		return Graph->FindNodeWithBindingObject(Value);
	};

	// Add connected to pin, if there is one
	if (FromPin != nullptr)
	{
		if (const UMDFastBindingGraphNode* GraphNode = Cast<UMDFastBindingGraphNode>(FromPin->GetOwningNode()))
		{
			if (UMDFastBindingObject* BindingObject = GraphNode->GetBindingObject())
			{
				FScopedTransaction Transaction = FScopedTransaction(LOCTEXT("SetBindingItem", "Set Binding Value"));
				BindingObject->Modify();
				if (UMDFastBindingValueBase* NewValue = BindingObject->SetBindingItem(FromPin->GetFName(), ValueClass))
				{
					return InitValueAndNode(NewValue);
				}
			}
		}
	}

	// Add as orphan instead
	if (UMDFastBindingDestinationBase* BindingDest = Graph->GetBindingDestination())
	{
		FScopedTransaction Transaction = FScopedTransaction(LOCTEXT("AddBindingNode", "Add Binding Node"));
		BindingDest->Modify();
		UMDFastBindingValueBase* NewValue = NewObject<UMDFastBindingValueBase>(BindingDest, ValueClass, NAME_None, RF_Public | RF_Transactional);
		if (UMDFastBindingValueBase* OrphanValue = BindingDest->AddOrphan(NewValue))
		{
			return InitValueAndNode(OrphanValue);
		}
	}

	return nullptr;
}

FLinearColor UMDFastBindingGraphSchema::GetPinTypeColor(const FEdGraphPinType& PinType) const
{
	if (PinType.PinCategory == NAME_None)
	{
		return FMDFastBindingEditorStyle::Get().GetColor(TEXT("InvalidPinColor"));
	}
	
	return GetDefault<UEdGraphSchema_K2>()->GetPinTypeColor(PinType);
}

FConnectionDrawingPolicy* UMDFastBindingGraphSchema::CreateConnectionDrawingPolicy(int32 InBackLayerID,
	int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements,
	UEdGraph* InGraphObj) const
{
	return new FKismetConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements, InGraphObj);
}

void UMDFastBindingGraphSchema::GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const
{
	static const FString CreateValueCategory = TEXT("Create Value Node...");
	TArray<TSubclassOf<UMDFastBindingValueBase>> ValueClasses;
	
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

	for (const TSubclassOf<UMDFastBindingValueBase>& ValueClass : ValueClasses)
	{
		ContextMenuBuilder.AddAction(MakeShared<FMDFastBindingSchemaAction_CreateValue>(ValueClass), CreateValueCategory);
	}
}

void UMDFastBindingGraphSchema::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotifcation) const
{
	auto OrphanValue = [](UMDFastBindingGraphNode* Node, const FName& BindingName)
	{
		if (Node == nullptr)
		{
			return;
		}
		
		if (UMDFastBindingObject* BindingObject = Node->GetBindingObject())
		{
			BindingObject->OrphanBindingItem(BindingName);
		}
	};
	
	// Use the input pin(s) to determine what to orphan
	if (TargetPin.Direction == EGPD_Input)
	{
		OrphanValue(Cast<UMDFastBindingGraphNode>(TargetPin.GetOwningNode()), TargetPin.GetFName());
	}
	else
	{
		for (const UEdGraphPin* ConnectedPin : TargetPin.LinkedTo)
		{
			OrphanValue(Cast<UMDFastBindingGraphNode>(ConnectedPin->GetOwningNode()), ConnectedPin->GetFName());
		}
	}
	
	Super::BreakPinLinks(TargetPin, bSendsNodeNotifcation);
}

const FPinConnectionResponse UMDFastBindingGraphSchema::CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const
{
	if (A->Direction != B->Direction)
	{
		const bool bAHasConnections = A->LinkedTo.Num() > 0;
		const bool bBHasConnections = B->LinkedTo.Num() > 0;
		
		if (bAHasConnections && bBHasConnections)
		{
			return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_AB, TEXT("Replace connections"));
		}
		
		if (bAHasConnections)
		{
			return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_A, TEXT("Replace connection"));
		}
		
		if (bBHasConnections)
		{
			return FPinConnectionResponse(CONNECT_RESPONSE_BREAK_OTHERS_B, TEXT("Replace connection"));
		}

		return FPinConnectionResponse(CONNECT_RESPONSE_MAKE, TEXT(""));
		
	}

	return Super::CanCreateConnection(A, B);
}

bool UMDFastBindingGraphSchema::TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const
{
	UEdGraphPin* InputPin = (A->Direction == EGPD_Input) ? A : B;
	UEdGraphPin* OutputPin = (A->Direction == EGPD_Output) ? A : B;

	TArray<UEdGraphPin*> OldOutputLinkedTo = OutputPin->LinkedTo;
	
	if (Super::TryCreateConnection(A, B))
	{
		const UMDFastBindingGraphNode* ValueNode = Cast<UMDFastBindingGraphNode>(OutputPin->GetOwningNode());
		if (ValueNode == nullptr)
		{
			return false;
		}
		
		UMDFastBindingValueBase* ValueObject = Cast<UMDFastBindingValueBase>(ValueNode->GetBindingObject());
		if (ValueObject == nullptr)
		{
			return false;
		}

		const UMDFastBindingGraphNode* InputNode = Cast<UMDFastBindingGraphNode>(InputPin->GetOwningNode());
		if (InputNode == nullptr)
		{
			return false;
		}
		
		UMDFastBindingObject* InputObject = InputNode->GetBindingObject();
		if (InputObject == nullptr)
		{
			return false;
		}

		InputObject->SetBindingItem(InputPin->GetFName(), ValueObject);

		// Delete the old ValueObject
		if (OldOutputLinkedTo.Num() == 0)
		{
			if (UMDFastBindingDestinationBase* BindingDest = ValueObject->GetOuterBindingDestination())
			{
				BindingDest->RemoveOrphan(ValueObject);
			}
		}
		else if (UEdGraphPin* OldOutputLinkedToPin = OldOutputLinkedTo[0])
		{
			if (const UMDFastBindingGraphNode* OldOutputLinkedToNode = Cast<UMDFastBindingGraphNode>(OldOutputLinkedToPin->GetOwningNode()))
			{
				if (UMDFastBindingObject* OldOutputLinkedToObject = OldOutputLinkedToNode->GetBindingObject())
				{
					OldOutputLinkedToObject->ClearBindingItemValue(OldOutputLinkedToPin->GetFName());
				}
			}
		}

		if (UMDFastBindingGraph* Graph = Cast<UMDFastBindingGraph>(InputNode->GetGraph()))
		{
			Graph->RefreshGraph();
		}
		
		return true;
	}

	return false;
}

#undef LOCTEXT_NAMESPACE
