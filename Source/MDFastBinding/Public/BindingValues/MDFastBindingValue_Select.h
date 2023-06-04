#pragma once

#include "MDFastBindingValueBase.h"
#include "MDFastBindingValue_Select.generated.h"

/**
 * Map a value to another value of a different type, similar to a Switch statement. Auto-populates for bool and enum inputs.
 */
UCLASS(meta = (DisplayName = "Select Value"))
class MDFASTBINDING_API UMDFastBindingValue_Select : public UMDFastBindingValueBase
{
	GENERATED_BODY()

public:

	virtual const FProperty* GetOutputProperty() override;

	virtual bool HasUserExtendablePinList() const override;

protected:
	virtual TTuple<const FProperty*, void*> GetValue_Internal(UObject* SourceObject) override;
	virtual void SetupBindingItems() override;
	virtual void SetupExtendablePinBindingItem(int32 ItemIndex) override;

#if WITH_EDITORONLY_DATA
	virtual FText GetDisplayName() override;
#endif
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(TArray<FText>& ValidationErrors) override;
#endif

private:
	const FProperty* ResolveOutputProperty();

	TWeakFieldPtr<const FProperty> ResolvedOutputProperty;

	TMap<int64, FName> EnumValueToPinNameMap;
};
