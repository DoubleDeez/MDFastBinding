#include "MDFastBindingGraphSchema.h"

#include "BlueprintConnectionDrawingPolicy.h"
#include "BindingDestinations/MDFastBindingDestinationBase.h"
#include "BindingValues/MDFastBindingValueBase.h"
#include "BindingValues/MDFastBindingValue_Property.h"
#include "EdGraphSchema_K2.h"
#include "MDFastBindingConnectionDrawingPolicy.h"
#include "MDFastBindingEditorStyle.h"
#include "MDFastBindingGraph.h"
#include "MDFastBindingGraphNode.h"
#include "MDFastBindingInstance.h"
#include "ScopedTransaction.h"

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

	const UMDFastBindingGraphNode* FromGraphNode = FromPin != nullptr ? Cast<UMDFastBindingGraphNode>(FromPin->GetOwningNode()) : nullptr;
	UMDFastBindingObject* FromBindingObject = FromGraphNode != nullptr ? FromGraphNode->GetBindingObject() : nullptr;

	// Add connected to input pin, if there is one
	if (FromPin != nullptr && FromPin->Direction == EGPD_Input && FromBindingObject != nullptr)
	{
		FScopedTransaction Transaction = FScopedTransaction(LOCTEXT("SetBindingItem", "Set Binding Value"));
		FromBindingObject->Modify();
		if (UMDFastBindingValueBase* NewValue = FromBindingObject->SetBindingItem(FromPin->GetFName(), ValueClass))
		{
			return InitValueAndNode(NewValue);
		}
	}

	// Add as orphan instead
	if (UMDFastBindingInstance* Binding = Graph->GetBinding())
	{
		FScopedTransaction Transaction = FScopedTransaction(LOCTEXT("AddBindingNode", "Add Binding Node"));
		Binding->Modify();
		UMDFastBindingValueBase* NewValue = NewObject<UMDFastBindingValueBase>(Binding, ValueClass, NAME_None, RF_Public | RF_Transactional);
		if (UMDFastBindingValueBase* OrphanValue = Binding->AddOrphan(NewValue))
		{
			if (UMDFastBindingGraphNode* NewNode = InitValueAndNode(OrphanValue))
			{
				// Connect output pin to first binding item in new node 
				if (FromPin != nullptr && FromPin->Direction == EGPD_Output && FromBindingObject != nullptr && NewNode->Pins.Num() > 0)
				{
					if (const UMDFastBindingGraphSchema* Schema = Cast<const UMDFastBindingGraphSchema>(Graph->GetSchema()))
					{
						Schema->TryCreateConnection(FromPin, NewNode->Pins[0]);
					}
				}

				return NewNode;
			}
		}
	}

	return nullptr;
}

FMDFastBindingSchemaAction_SetDestination::FMDFastBindingSchemaAction_SetDestination(TSubclassOf<UMDFastBindingDestinationBase> InDestinationClass)
	: DestinationClass(InDestinationClass)
{
	UpdateSearchData(DestinationClass->GetDisplayNameText()
		, DestinationClass->GetToolTipText()
		, FText::GetEmpty()
		, FText::GetEmpty());
}

FName FMDFastBindingSchemaAction_SetDestination::GetTypeId() const
{
	return DestinationClass->GetFName();
}

UEdGraphNode* FMDFastBindingSchemaAction_SetDestination::PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode)
{
	UMDFastBindingGraph* Graph = Cast<UMDFastBindingGraph>(ParentGraph);
	if (Graph == nullptr || DestinationClass == nullptr)
	{
		return nullptr;
	}

	const UMDFastBindingGraphNode* FromGraphNode = FromPin != nullptr ? Cast<UMDFastBindingGraphNode>(FromPin->GetOwningNode()) : nullptr;
	UMDFastBindingObject* FromBindingObject = FromGraphNode != nullptr ? FromGraphNode->GetBindingObject() : nullptr;

	if (UMDFastBindingInstance* Binding = Graph->GetBinding())
	{
		FScopedTransaction Transaction = FScopedTransaction(LOCTEXT("SetBindingDestinationNode", "Set Binding Destination"));
		Binding->Modify();
		if (UMDFastBindingDestinationBase* NewDestination = Binding->SetDestination(DestinationClass))
		{
			NewDestination->NodePos = Location.IntPoint();
			Graph->ClearSelection();
			Graph->RefreshGraph();
			Graph->SelectNodeWithBindingObject(NewDestination);
			if (UMDFastBindingGraphNode* NewNode =  Graph->FindNodeWithBindingObject(NewDestination))
			{
				// Connect output pin to first binding item in new node 
				if (FromPin != nullptr && FromPin->Direction == EGPD_Output && FromBindingObject != nullptr && NewNode->Pins.Num() > 0)
				{
					if (const UMDFastBindingGraphSchema* Schema = Cast<const UMDFastBindingGraphSchema>(Graph->GetSchema()))
					{
						Schema->TryCreateConnection(FromPin, NewNode->Pins[0]);
					}
				}

				return NewNode;
			}
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
	return new FMDFastBindingConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, InZoomFactor, InClippingRect, InDrawElements, InGraphObj);
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

void UMDFastBindingGraphSchema::BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotification) const
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
	
	Super::BreakPinLinks(TargetPin, bSendsNodeNotification);
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
			if (UMDFastBindingInstance* Binding = ValueObject->GetOuterBinding())
			{
				Binding->RemoveOrphan(ValueObject);
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

bool UMDFastBindingGraphSchema::CanVariableBeDropped(UEdGraph* InGraph, FProperty* InVariableToDrop) const
{
	if (UMDFastBindingGraph* Graph = Cast<UMDFastBindingGraph>(InGraph))
	{
		if (UMDFastBindingInstance* Binding = Graph->GetBinding())
		{
			if (UClass* OwnerClass = Binding->GetBindingOuterClass())
			{
				for (TFieldIterator<FProperty> It(OwnerClass); It; ++It)
				{
					const FProperty* Prop = *It;
					if (Prop->GetFName() == InVariableToDrop->GetFName())
					{
						return true;
					}
				}
			}
		}
	}
	
	return Super::CanVariableBeDropped(InGraph, InVariableToDrop);
}

bool UMDFastBindingGraphSchema::RequestVariableDropOnPanel(UEdGraph* InGraph, FProperty* InVariableToDrop, const FVector2D& InDropPosition, const FVector2D& InScreenPosition)
{
	return RequestVariableDropOnPin(InGraph, InVariableToDrop, nullptr, InDropPosition, InScreenPosition);
}

bool UMDFastBindingGraphSchema::RequestVariableDropOnNode(UEdGraph* InGraph, FProperty* InVariableToDrop, UEdGraphNode* InNode, const FVector2D& InDropPosition, const FVector2D& InScreenPosition)
{
	// Treat dropping on node the same as dropping on panel
	return RequestVariableDropOnPanel(InGraph, InVariableToDrop, InDropPosition, InScreenPosition);
}

bool UMDFastBindingGraphSchema::RequestVariableDropOnPin(UEdGraph* InGraph, FProperty* InVariableToDrop, UEdGraphPin* InPin, const FVector2D& InDropPosition, const FVector2D& InScreenPosition)
{
	if (CanVariableBeDropped(InGraph, InVariableToDrop))
	{
		const FVector2D NodeOffset = [&]()
		{
			if (InPin == nullptr)
			{
				return FVector2D::ZeroVector;
			}

			return InPin->Direction == EGPD_Input ? FVector2D(-200, 0) : FVector2D(200, 0);
		}();
		
		FMDFastBindingSchemaAction_CreateValue CreateAction = FMDFastBindingSchemaAction_CreateValue(UMDFastBindingValue_Property::StaticClass());
		if (UMDFastBindingGraphNode* Node = Cast<UMDFastBindingGraphNode>(CreateAction.PerformAction(InGraph, InPin, InDropPosition + NodeOffset)))
		{
			if (UMDFastBindingValue_Property* PropertyValue = Cast<UMDFastBindingValue_Property>(Node->GetBindingObject()))
			{
				PropertyValue->SetFieldPath({ InVariableToDrop });
			
				if (UMDFastBindingGraph* Graph = Cast<UMDFastBindingGraph>(InGraph))
				{
					Graph->RefreshGraph();
					Graph->SelectNodeWithBindingObject(PropertyValue);
				}
				return true;
			}
		}
	}
	
	return Super::RequestVariableDropOnPin(InGraph, InVariableToDrop, InPin, InDropPosition, InScreenPosition);
}

#undef LOCTEXT_NAMESPACE
