﻿#pragma once

#include "EdGraph/EdGraph.h"
#include "MDFastBindingGraph.generated.h"

class UMDFastBindingObject;
class UMDFastBindingInstance;
class UMDFastBindingGraphNode;
class SMDFastBindingEditorGraphWidget;

/**
 *
 */
UCLASS()
class MDFASTBINDINGBLUEPRINT_API UMDFastBindingGraph : public UEdGraph
{
	GENERATED_BODY()

public:
	void SetGraphWidget(TSharedRef<SMDFastBindingEditorGraphWidget> InGraphWidget);

	void RefreshGraph();

	void SetBinding(UMDFastBindingInstance* InBinding);
	UMDFastBindingInstance* GetBinding() const { return Binding.Get(); }

	void SetBindingBeingDebugged(UMDFastBindingInstance* InBinding);
	UMDFastBindingInstance* GetBindingBeingDebugged() const { return BindingBeingDebugged.Get(); }

	UMDFastBindingGraphNode* FindNodeWithBindingObject(UMDFastBindingObject* InObject) const;
	void SelectNodeWithBindingObject(UMDFastBindingObject* InObject);

	void ClearSelection();

	UBlueprint* GetBlueprint() const;

private:
	TWeakObjectPtr<UMDFastBindingInstance> Binding;
	TWeakObjectPtr<UMDFastBindingInstance> BindingBeingDebugged;

	TWeakPtr<SMDFastBindingEditorGraphWidget> GraphWidget;
};
