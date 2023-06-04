#pragma once

#include "EdGraph/EdGraphSchema.h"
#include "Templates/SubclassOf.h"
#include "MDFastBindingGraphSchema.generated.h"

class UMDFastBindingValueBase;
class UMDFastBindingDestinationBase;

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

USTRUCT()
struct FMDFastBindingSchemaAction_SetDestination : public FEdGraphSchemaAction
{
	GENERATED_BODY()

public:
	FMDFastBindingSchemaAction_SetDestination() = default;

	explicit FMDFastBindingSchemaAction_SetDestination(TSubclassOf<UMDFastBindingDestinationBase> InDestinationClass);

	UPROPERTY()
	TSubclassOf<UMDFastBindingDestinationBase> DestinationClass;

	virtual FName GetTypeId() const override;

	virtual UEdGraphNode* PerformAction(UEdGraph* ParentGraph, UEdGraphPin* FromPin, const FVector2D Location, bool bSelectNewNode = true) override;
};

/**
 *
 */
UCLASS()
class MDFASTBINDINGBLUEPRINT_API UMDFastBindingGraphSchema : public UEdGraphSchema
{
	GENERATED_BODY()

public:
	virtual FLinearColor GetPinTypeColor(const FEdGraphPinType& PinType) const override;

	virtual FConnectionDrawingPolicy* CreateConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float InZoomFactor, const FSlateRect& InClippingRect, class FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj) const override;

	virtual void GetGraphContextActions(FGraphContextMenuBuilder& ContextMenuBuilder) const override;

	virtual void BreakPinLinks(UEdGraphPin& TargetPin, bool bSendsNodeNotification) const override;

	virtual const FPinConnectionResponse CanCreateConnection(const UEdGraphPin* A, const UEdGraphPin* B) const override;

	virtual bool TryCreateConnection(UEdGraphPin* A, UEdGraphPin* B) const override;

	virtual bool CanVariableBeDropped(UEdGraph* InGraph, FProperty* InVariableToDrop) const override;

	virtual bool RequestVariableDropOnPanel(UEdGraph* InGraph, FProperty* InVariableToDrop, const FVector2D& InDropPosition, const FVector2D& InScreenPosition) override;

	virtual bool RequestVariableDropOnNode(UEdGraph* InGraph, FProperty* InVariableToDrop, UEdGraphNode* InNode, const FVector2D& InDropPosition, const FVector2D& InScreenPosition) override;

	virtual bool RequestVariableDropOnPin(UEdGraph* InGraph, FProperty* InVariableToDrop, UEdGraphPin* InPin, const FVector2D& InDropPosition, const FVector2D& InScreenPosition) override;

	virtual void GetGraphDisplayInformation(const UEdGraph& Graph, /*out*/ FGraphDisplayInfo& DisplayInfo) const override;
};
