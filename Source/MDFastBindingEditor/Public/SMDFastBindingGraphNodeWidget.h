#pragma once

#include "CoreMinimal.h"
#include "SGraphNode.h"
#include "SGraphPin.h"
#include "KismetPins/SGraphPinObject.h"

class UMDFastBindingGraphNode;

class MDFASTBINDINGEDITOR_API SMDFastBindingSelfGraphPinWidget : public SGraphPinObject
{
public:
	using SGraphPinObject::Construct;

	virtual bool ShouldDisplayAsSelfPin() const override { return true; }
};

/**
 * 
 */
class MDFASTBINDINGEDITOR_API SMDFastBindingGraphNodeWidget : public SGraphNode
{
public:
	SLATE_BEGIN_ARGS(SMDFastBindingGraphNodeWidget)
		{
		}

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, UMDFastBindingGraphNode* InNode);

#if ENGINE_MAJOR_VERSION <= 4
	virtual void MoveTo( const FVector2D& NewPosition, FNodeSet& NodeFilter, bool bMarkDirty = true ) override;
#else
	virtual void MoveTo(const FVector2D& NewPosition, FNodeSet& NodeFilter) override;
#endif

	UMDFastBindingGraphNode* GetGraphNode() const;

protected:
	virtual void UpdateErrorInfo() override;

	virtual TSharedPtr<SGraphPin> CreatePinWidget(UEdGraphPin* Pin) const override;

	bool IsSelfPin(UEdGraphPin& Pin) const;
	
};
