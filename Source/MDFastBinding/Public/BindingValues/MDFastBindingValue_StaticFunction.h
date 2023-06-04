#pragma once

#include "MDFastBindingValue_Function.h"
#include "MDFastBindingValue_StaticFunction.generated.h"

/**
 * Call a static function (eg. blueprint library function) and retrieve its return value
 */
UCLASS(meta = (DisplayName = "Call a Static Function"))
class MDFASTBINDING_API UMDFastBindingValue_StaticFunction : public UMDFastBindingValue_Function
{
	GENERATED_BODY()

public:
	UMDFastBindingValue_StaticFunction();

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(TArray<FText>& ValidationErrors) override;
#endif

protected:
	virtual UObject* GetFunctionOwner(UObject* SourceObject) override;
	virtual UClass* GetFunctionOwnerClass() override { return FunctionOwnerClass; }
	virtual bool IsFunctionValid(UFunction* Func, const FProperty* ReturnValue, const TArray<const FProperty*>& Params) const override;

	UPROPERTY(EditDefaultsOnly, Category = "Binding")
	TSubclassOf<UObject> FunctionOwnerClass;
};
