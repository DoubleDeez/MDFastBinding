#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphSchema.h"
#include "MDFastBindingGraphSchema.generated.h"

class UMDFastBindingValueBase;

USTRUCT()
struct FMDFastBindingSchemaAction_CreateValue : public FEdGraphSchemaAction
{
	GENERATED_BODY()

public:
	FMDFastBindingSchemaAction_CreateValue() = default;
	
	explicit FMDFastBindingSchemaAction_CreateValue(TSubclassOf<UMDFastBindingValueBase> InValueClass);

	UPROPERTY()
	TSubclassOf<UMDFastBindingValueBase> ValueClass;

	virtual FName GetTypeId() const override;

	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
};

/**
 * 
 */
UCLASS()
class MDFASTBINDINGEDITOR_API UMDFastBindingGraphSchema : public UEdGraphSchema
{
	GENERATED_BODY()

public:
	virtual FLinearColor GetPinTypeColor(const FEdGraphPinType& PinType) const override;
	
	virtual FConnectionDrawingPolicy* CreateConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, class FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj) const override;
	
	virtual void GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const override;
};
