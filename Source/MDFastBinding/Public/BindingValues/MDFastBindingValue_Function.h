#pragma once

#include "MDFastBindingFunctionWrapper.h"
#include "MDFastBindingValueBase.h"
#include "UObject/WeakFieldPtr.h"
#include "MDFastBindingValue_Function.generated.h"

/**
 * Call a function and retrieve its return value
 */
UCLASS(meta = (DisplayName = "Call a Function"))
class MDFASTBINDING_API UMDFastBindingValue_Function : public UMDFastBindingValueBase
{
	GENERATED_BODY()

public:
	UMDFastBindingValue_Function();

	virtual const FProperty* GetOutputProperty() override;

#if WITH_EDITORONLY_DATA
	virtual bool DoesBindingItemDefaultToSelf(const FName& InItemName) const override;
	virtual bool IsBindingItemWorldContextObject(const FName& InItemName) const override;
	virtual FText GetDisplayName() override;
#endif

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(TArray<FText>& ValidationErrors) override;

	virtual void OnVariableRenamed(UClass* VariableClass, const FName& OldVariableName, const FName& NewVariableName) override;

	UFunction* GetFunction();
#endif

protected:
	virtual TTuple<const FProperty*, void*> GetValue_Internal(UObject* SourceObject) override;
	virtual UObject* GetFunctionOwner(UObject* SourceObject);
	virtual UClass* GetFunctionOwnerClass();
	virtual void PopulateFunctionParam(UObject* SourceObject, const FProperty* Param, void* ValuePtr);
	virtual bool IsFunctionValid(UFunction* Func, const TWeakFieldPtr<const FProperty>& ReturnValue, const TArray<TWeakFieldPtr<const FProperty>>& Params) const;

	virtual void SetupBindingItems() override;

	virtual void PostInitProperties() override;

	virtual bool ShouldCallFunction();

	UPROPERTY(EditDefaultsOnly, Category = "Binding")
	FMDFastBindingFunctionWrapper Function;

	UPROPERTY(Transient)
	UObject* ObjectProperty = nullptr;

	UPROPERTY(Transient)
	bool bAddPathRootBindingItem = true;

	UPROPERTY(Transient)
	bool bNeedsUpdate = false;
};
