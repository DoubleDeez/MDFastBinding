#pragma once

#include "CoreMinimal.h"
#include "SGraphNode.h"

class UMDFastBindingGraphNode;
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

	virtual void MoveTo(const FVector2D& NewPosition, FNodeSet& NodeFilter) override;

	UMDFastBindingGraphNode* GetGraphNode() const;

protected:
	virtual void UpdateErrorInfo() override;
	
};
