#pragma once

#include "BlueprintConnectionDrawingPolicy.h"

class FMDFastBindingConnectionDrawingPolicy : public FKismetConnectionDrawingPolicy
{
public:
	FMDFastBindingConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj);

	virtual bool TreatWireAsExecutionPin(UEdGraphPin* InputPin, UEdGraphPin* OutputPin) const override { return true; }

	virtual void DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, /*inout*/ FConnectionParams& Params) override;

	virtual void BuildExecutionRoadmap() override;

	virtual bool CanBuildRoadmap() const override;
};
