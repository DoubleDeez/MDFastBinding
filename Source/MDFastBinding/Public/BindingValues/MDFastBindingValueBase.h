#pragma once

#include "CoreMinimal.h"
#include "MDFastBindingObject.h"
#include "MDFastBindingValueBase.generated.h"

class UMDFastBindingDestinationBase;

/**
 * 
 */
UCLASS(Abstract, EditInlineNew, DefaultToInstanced)
class MDFASTBINDING_API UMDFastBindingValueBase : public UMDFastBindingObject
{
	GENERATED_BODY()

public:
	virtual TTuple<const FProperty*, void*> GetValue(UObject* SourceObject) { PURE_VIRTUAL(UMDFastBindingValueBase::GetValue, return {};) }

	virtual const FProperty* GetOutputProperty() { PURE_VIRTUAL(UMDFastBindingValueBase::GetValue, return nullptr;) }

	UMDFastBindingDestinationBase* GetOuterBindingDestination() const;

#if WITH_EDITORONLY_DATA
	void OrphanAllBindingItems(const TSet<UObject*>& OrphanExclusionSet);
	virtual void OrphanBindingItem(UMDFastBindingValueBase* InValue) override;
#endif

private:
	mutable TWeakObjectPtr<UMDFastBindingDestinationBase> OuterBindingDestination;
	
};
