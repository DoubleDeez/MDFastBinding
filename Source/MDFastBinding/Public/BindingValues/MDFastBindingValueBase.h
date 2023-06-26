#pragma once

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
	virtual void BeginDestroy() override;

	void InitializeValue(UObject* SourceObject);
	void TerminateValue(UObject* SourceObject);

	TTuple<const FProperty*, void*> GetValue(UObject* SourceObject, bool& OutDidUpdate);
#if WITH_EDITOR
	TTuple<const FProperty*, void*> GetCachedValue() const { return CachedValue; }
#endif

	virtual const FProperty* GetOutputProperty() { PURE_VIRTUAL(UMDFastBindingValueBase::GetValue, return nullptr;) }

	const FMDFastBindingItem* GetOwningBindingItem() const;

protected:
	virtual bool CheckNeedsUpdate() const override;

	virtual void InitializeValue_Internal(UObject* SourceObject) {}
	virtual TTuple<const FProperty*, void*> GetValue_Internal(UObject* SourceObject) { PURE_VIRTUAL(UMDFastBindingValueBase::GetValue, return {};) }
	virtual void TerminateValue_Internal(UObject* SourceObject) {}

private:
	TTuple<const FProperty*, void*> CachedValue;

};
