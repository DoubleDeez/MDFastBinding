#pragma once

#include "EdGraph/EdGraphNode.h"
#include "MDFastBindingGraphNode.generated.h"

class UMDFastBindingInstance;
class UMDFastBindingObject;

/**
 *
 */
UCLASS()
class MDFASTBINDINGBLUEPRINT_API UMDFastBindingGraphNode : public UEdGraphNode
{
	GENERATED_BODY()

public:
	UMDFastBindingGraphNode();

	virtual void AllocateDefaultPins() override;

	void SetBindingObject(UMDFastBindingObject* InObject);

	UMDFastBindingObject* GetBindingObject() const { return BindingObject.Get(); }
	UMDFastBindingObject* GetCopiedBindingObject() const { return CopiedObject; }

	void SetBindingBeingDebugged(UMDFastBindingInstance* InBinding);
	UMDFastBindingObject* GetBindingObjectBeingDebugged() const;

	UMDFastBindingGraphNode* GetLinkedOutputNode() const;

	void ClearConnection(const FName& PinName);

	void OnMoved();

	void DeleteNode(const TSet<UObject*>& OrphanExclusionSet);

	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;

	virtual void PrepareForCopying() override;

	virtual FString GetPinMetaData(FName InPinName, FName InKey) override;

	virtual FText GetPinDisplayName(const UEdGraphPin* Pin) const override;

	void CleanUpCopying();
	void CleanUpPasting();

	void RefreshGraph();

	UBlueprint* GetBlueprint() const;

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
	UPROPERTY(VisibleAnywhere, Transient, Instanced, Category = "Fast Binding")
	TObjectPtr<UMDFastBindingObject> BindingObject;
	TWeakObjectPtr<UMDFastBindingInstance> BindingBeingDebugged;

	UPROPERTY()
	TObjectPtr<UMDFastBindingObject> CopiedObject = nullptr;
};
