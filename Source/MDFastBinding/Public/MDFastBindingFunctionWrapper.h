#pragma once

#include "CoreMinimal.h"
#include "MDFastBindingFunctionWrapper.generated.h"

DECLARE_DELEGATE_RetVal_OneParam(UObject*, FMDGetFunctionOwner, UObject*);
DECLARE_DELEGATE_RetVal(UClass*, FMDGetFunctionOwnerClass);
DECLARE_DELEGATE_ThreeParams(FMDPopulateFunctionParam, UObject*, const FProperty*, void*);
DECLARE_DELEGATE_RetVal_ThreeParams(bool, FMDFunctionFilter, UFunction*, const FProperty*, const TArray<const FProperty*>&);
DECLARE_DELEGATE_RetVal(bool, FMDShouldCallFunction)

/**
 * 
 */
USTRUCT()
struct MDFASTBINDING_API FMDFastBindingFunctionWrapper
{
	GENERATED_BODY()

public:
	~FMDFastBindingFunctionWrapper();

	bool BuildFunctionData();
	
	UClass* GetFunctionOwnerClass() const;
	
	const TArray<const FProperty*>& GetParams();

	const FProperty* GetReturnProp();

	UFunction* GetFunctionPtr();

	TTuple<const FProperty*, void*> CallFunction(UObject* SourceObject);

	const FName& GetFunctionName() const { return FunctionName; }

#if WITH_EDITORONLY_DATA
	FString ToString();

	static FString FunctionToString(UFunction* Func);
	
	static FString FunctionToString_Internal(UFunction* Func, const FProperty* ReturnProp, const TArray<const FProperty*>& Params);
#endif

	static bool IsFunctionValidForWrapper(const UFunction* Func);
	
	FMDGetFunctionOwner OwnerGetter;
	FMDGetFunctionOwnerClass OwnerClassGetter;
	FMDPopulateFunctionParam ParamPopulator;
	FMDFunctionFilter FunctionFilter;

	// Last chance to opt-out of calling the function (ie, if none of the params updated)
	FMDShouldCallFunction ShouldCallFunction;

	// TODO - store GUID so we can check for renames
	UPROPERTY(EditAnywhere, Category = "Bindings")
	FName FunctionName = NAME_None;

private:
	UPROPERTY(Transient)
	UFunction* FunctionPtr = nullptr;

	TArray<const FProperty*> Params;

	const FProperty* ReturnProp = nullptr;
	
	void* FunctionMemory = nullptr;
	UObject* GetFunctionOwner(UObject* SourceObject) const;
	void InitFunctionMemory();
	void PopulateParams(UObject* SourceObject);
};
