#include "MDFastBindingGraphSchema.h"

#include "BlueprintConnectionDrawingPolicy.h"
#include "MDFastBindingEditorStyle.h"
#include "MDFastBindingGraph.h"
#include "MDFastBindingGraphNode.h"
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
	if (FromPin == nullptr || Graph == nullptr)
	{
		return nullptr;
	}

	if (const UMDFastBindingGraphNode* GraphNode = Cast<UMDFastBindingGraphNode>(FromPin->GetOwningNode()))
	{
		if (UMDFastBindingObject* BindingObject = GraphNode->GetBindingObject())
		{
			FScopedTransaction Transaction = FScopedTransaction(LOCTEXT("SetBindingItem", "Set Binding Value"));
			BindingObject->Modify();
			if (UMDFastBindingValueBase* NewValue = BindingObject->SetBindingItem(FromPin->GetFName(), ValueClass))
			{
				NewValue->NodePos = Location.IntPoint();
				Graph->ClearSelection();
				Graph->RefreshGraph();
				Graph->SelectNodeWithBindingObject(NewValue);
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

#undef LOCTEXT_NAMESPACE
