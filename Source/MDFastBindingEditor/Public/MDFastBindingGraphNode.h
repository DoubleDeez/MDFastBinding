#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraphNode.h"
#include "MDFastBindingGraphNode.generated.h"

class UMDFastBindingObject;

/**
 * 
 */
UCLASS()
class MDFASTBINDINGEDITOR_API UMDFastBindingGraphNode : public UEdGraphNode
{
	GENERATED_BODY()

public:
	UMDFastBindingGraphNode();
	
	virtual void AllocateDefaultPins() override;
	
	void SetBindingObject(UMDFastBindingObject* InObject);

	UMDFastBindingObject* GetBindingObject() const { return BindingObject.Get(); }

	void ClearConnection(const FName& PinName);

	void OnMoved();

	void DeleteNode();

	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;

	static const FName OutputPinName;

protected:
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;

	virtual TSharedPtr<SGraphNode> CreateVisualWidget() override;
	
	virtual void OnRenameNode(const FString& NewName) override;

	virtual void OnUpdateCommentText(const FString& NewComment) override;

	virtual void OnCommentBubbleToggled(bool bInCommentBubbleVisible) override;

	virtual FLinearColor GetNodeTitleColor() const override;
	
	virtual FText GetTooltipText() const override;

private:
	TWeakObjectPtr<UMDFastBindingObject> BindingObject;
};
