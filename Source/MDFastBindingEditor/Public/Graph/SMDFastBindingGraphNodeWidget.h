#pragma once

#include "CoreMinimal.h"
#include "SGraphNode.h"
#include "SGraphPin.h"
#include "KismetPins/SGraphPinObject.h"

class SMDFastBindingPinValueInspector;
class SMDFastBindingGraphNodeWidget;
class UMDFastBindingGraphNode;
class UMDFastBindingObject;

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
	
	virtual void MoveTo(const FVector2D& NewPosition, FNodeSet& NodeFilter, bool bMarkDirty = true) override;
	
	virtual void GetOverlayBrushes(bool bSelected, const FVector2D WidgetSize, TArray<FOverlayBrushInfo>& Brushes) const override;

	UMDFastBindingGraphNode* GetGraphNode() const;

	UMDFastBindingObject* GetBindingObject() const;

	UMDFastBindingObject* GetBindingObjectBeingDebugged() const;

protected:
	virtual void UpdateErrorInfo() override;

	virtual TSharedPtr<SGraphPin> CreatePinWidget(UEdGraphPin* Pin) const override;
	
	virtual void CreateBelowPinControls(TSharedPtr<SVerticalBox> MainBox) override;

	virtual void CreateInputSideAddButton(TSharedPtr<SVerticalBox> InputBox) override;

	virtual FReply OnAddPin() override;

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	bool IsSelfPin(UEdGraphPin& Pin) const;

	void UpdateDebugTooltip(double InCurrentTime);

	void SetDebugTooltipPin(const FGeometry& PinGeometry, UEdGraphPin* Pin, UMDFastBindingObject* DebugObject);
	void ClearPinDebugTooltip();

	void CalculatePinTooltipLocation(const FVector2D& Offset, FVector2D& OutTooltipLocation);

private:
	TWeakPtr<FPinValueInspectorTooltip> PinValueInspectorTooltip;
	TWeakPtr<SMDFastBindingPinValueInspector> PinValueInspectorPtr;

	double UnhoveredTooltipCloseTime = 0.0;

	constexpr static double CloseTooltipGracePeriod = 0.4;
	
};
