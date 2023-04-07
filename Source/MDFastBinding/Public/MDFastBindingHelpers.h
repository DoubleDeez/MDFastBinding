#pragma once

#include "CoreMinimal.h"

class UWidgetBlueprintGeneratedClass;

class MDFASTBINDING_API FMDFastBindingHelpers
{
public:
	static void GetFunctionParamProps(const UFunction* Func, TArray<const FProperty*>& OutParams);
	static void SplitFunctionParamsAndReturnProp(const UFunction* Func, TArray<const FProperty*>& OutParams, const FProperty*& OutReturnProp);

	static FString PropertyToString(const FProperty& Prop);

	static bool ArePropertyValuesEqual(const FProperty* PropA, const void* ValuePtrA, const FProperty* PropB, const void* ValuePtrB);
	
	static bool DoesClassHaveSuperClassBindings(UWidgetBlueprintGeneratedClass* Class);
};
