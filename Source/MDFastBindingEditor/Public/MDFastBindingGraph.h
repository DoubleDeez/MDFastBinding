#pragma once

#include "CoreMinimal.h"
#include "EdGraph/EdGraph.h"
#include "MDFastBindingGraph.generated.h"

class UMDFastBindingObject;
class UMDFastBindingDestinationBase;
class UMDFastBindingGraphNode;
class SMDFastBindingEditorGraphWidget;

/**
 * 
 */
UCLASS()
class MDFASTBINDINGEDITOR_API UMDFastBindingGraph : public UEdGraph
{
	GENERATED_BODY()

public:
	void SetGraphWidget(TSharedRef<SMDFastBindingEditorGraphWidget> InGraphWidget);
	
	void RefreshGraph();
	
	void SetBindingDestination(UMDFastBindingDestinationBase* InBinding);
	UMDFastBindingDestinationBase* GetBindingDestination() const { return Binding.Get(); }

	UMDFastBindingGraphNode* FindNodeWithBindingObject(UMDFastBindingObject* InObject) const;
	void SelectNodeWithBindingObject(UMDFastBindingObject* InObject);

	void ClearSelection();

private:
	TWeakObjectPtr<UMDFastBindingDestinationBase> Binding;

	TWeakPtr<SMDFastBindingEditorGraphWidget> GraphWidget;
};
