#pragma once

#include "CoreMinimal.h"
#include "MDFastBindingObject.h"
#include "MDFastBindingValueBase.generated.h"

class UMDFastBindingInstance;

/**
 * 
 */
UCLASS(Abstract, EditInlineNew, DefaultToInstanced)
class MDFASTBINDING_API UMDFastBindingValueBase : public UMDFastBindingObject
{
	GENERATED_BODY()

public:
	void InitializeValue(UObject* SourceObject);
	
	TTuple<const FProperty*, void*> GetValue(UObject* SourceObject, bool& OutDidUpdate);
	
	virtual bool CheckNeedsUpdate() const override;

	virtual const FProperty* GetOutputProperty() { PURE_VIRTUAL(UMDFastBindingValueBase::GetValue, return nullptr;) }

	const FMDFastBindingItem* GetOwningBindingItem() const;

protected:
	virtual TTuple<const FProperty*, void*> GetValue_Internal(UObject* SourceObject) { PURE_VIRTUAL(UMDFastBindingValueBase::GetValue, return {};) }

private:
	TTuple<const FProperty*, void*> CachedValue;
	
};
