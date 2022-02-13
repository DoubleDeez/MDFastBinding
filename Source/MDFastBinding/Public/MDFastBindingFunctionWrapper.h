#pragma once

#include "CoreMinimal.h"
#include "MDFastBindingFunctionWrapper.generated.h"

DECLARE_DELEGATE_RetVal_OneParam(UObject*, FMDGetFunctionOwner, UObject*);
DECLARE_DELEGATE_RetVal(UClass*, FMDGetFunctionOwnerClass);
DECLARE_DELEGATE_ThreeParams(FMDPopulateFunctionParam, UObject*, const FProperty*, void*);
DECLARE_DELEGATE_RetVal_ThreeParams(bool, FMDFunctionFilter, UFunction*, const FProperty*, const TArray<const FProperty*>&);

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

	TTuple<const FProperty*, void*> CallFunction(UObject* SourceObject);

	const FName& GetFunctionName() const { return FunctionName; }

	FString ToString();

	static FString FunctionToString(UFunction* Func);

	static bool IsFunctionValidForWrapper(const UFunction* Func);
	
	FMDGetFunctionOwner OwnerGetter;
	FMDGetFunctionOwnerClass OwnerClassGetter;
	FMDPopulateFunctionParam ParamPopulator;
	FMDFunctionFilter FunctionFilter;

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
	
	static FString FunctionToString_Internal(UFunction* Func, const FProperty* ReturnProp, const TArray<const FProperty*>& Params);
};
