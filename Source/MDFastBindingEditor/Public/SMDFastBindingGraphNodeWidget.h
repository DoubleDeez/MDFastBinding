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

#if ENGINE_MAJOR_VERSION <= 4
	virtual void MoveTo( const FVector2D& NewPosition, FNodeSet& NodeFilter, bool bMarkDirty = true ) override;
#else
	virtual void MoveTo(const FVector2D& NewPosition, FNodeSet& NodeFilter) override;
#endif

	UMDFastBindingGraphNode* GetGraphNode() const;

protected:
	virtual void UpdateErrorInfo() override;
	
};
