﻿#pragma once

#include "MDFastBindingFunctionWrapper.h"
#include "BindingDestinations/MDFastBindingDestinationBase.h"
#include "MDFastBindingDestination_Function.generated.h"

/**
 * Call a function on an object
 */
UCLASS(collapseCategories, meta=(DisplayName = "Call a Function"))
class MDFASTBINDING_API UMDFastBindingDestination_Function : public UMDFastBindingDestinationBase
{
	GENERATED_BODY()

public:
#if WITH_EDITORONLY_DATA
	virtual bool DoesBindingItemDefaultToSelf(const FName& InItemName) const override;
	virtual bool IsBindingItemWorldContextObject(const FName& InItemName) const override;
#endif

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(TArray<FText>& ValidationErrors) override;

	virtual void OnVariableRenamed(UClass* VariableClass, const FName& OldVariableName, const FName& NewVariableName) override;

	UFunction* GetFunction();
	void SetFunction(UFunction* Func);
#endif

#if WITH_EDITORONLY_DATA
	virtual FText GetDisplayName() override;
#endif

protected:
	virtual void InitializeDestination_Internal(UObject* SourceObject) override;
	virtual void UpdateDestination_Internal(UObject* SourceObject) override;

	virtual UObject* GetFunctionOwner(UObject* SourceObject);
	virtual UClass* GetFunctionOwnerClass();
	virtual void PopulateFunctionParam(UObject* SourceObject, const FProperty* Param, void* ValuePtr);

	virtual void SetupBindingItems() override;

	virtual void PostInitProperties() override;

	virtual bool ShouldCallFunction();

protected:
	UPROPERTY(EditAnywhere, Category = "Binding")
	FMDFastBindingFunctionWrapper Function;

private:
	UPROPERTY(Transient)
	UObject* ObjectProperty = nullptr;

	UPROPERTY(Transient)
	bool bNeedsUpdate = false;
};
