#pragma once

#include "CoreMinimal.h"

class MDFASTBINDING_API FMDFastBindingHelpers
{
public:
	static void GetFunctionParamProps(const UFunction* Func, TArray<const FProperty*>& OutParams);
	static void SplitFunctionParamsAndReturnProp(const UFunction* Func, TArray<const FProperty*>& OutParams, const FProperty*& OutReturnProp);
};
