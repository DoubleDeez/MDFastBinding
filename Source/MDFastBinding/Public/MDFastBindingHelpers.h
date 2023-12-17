#pragma once

#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "UObject/WeakFieldPtr.h"


class FProperty;
class UFunction;
class UWidgetBlueprintGeneratedClass;

class MDFASTBINDING_API FMDFastBindingHelpers
{
public:
	static void GetFunctionParamProps(const UFunction* Func, TArray<const FProperty*>& OutParams);
	static void SplitFunctionParamsAndReturnProp(const UFunction* Func, TArray<TWeakFieldPtr<const FProperty>>& OutParams, TWeakFieldPtr<const FProperty>& OutReturnProp);

	static FString PropertyToString(const FProperty& Prop);

	static bool ArePropertyValuesEqual(const FProperty* PropA, const void* ValuePtrA, const FProperty* PropB, const void* ValuePtrB);

	static bool DoesClassHaveSuperClassBindings(UWidgetBlueprintGeneratedClass* Class);
};
