#include "BindingValues/MDFastBindingValue_StaticFunction.h"

#define LOCTEXT_NAMESPACE "MDFastBindingDestination_StaticFunction"

UMDFastBindingValue_StaticFunction::UMDFastBindingValue_StaticFunction()
{
	bAddPathRootBindingItem = false;
}

UObject* UMDFastBindingValue_StaticFunction::GetFunctionOwner(UObject* SourceObject)
{
	return FunctionOwnerClass != nullptr ? FunctionOwnerClass.GetDefaultObject() : nullptr;
}

bool UMDFastBindingValue_StaticFunction::IsFunctionValid(UFunction* Func, const TWeakFieldPtr<const FProperty>& ReturnValue, const TArray<TWeakFieldPtr<const FProperty>>& Params) const
{
	return Super::IsFunctionValid(Func, ReturnValue, Params) && Func != nullptr && Func->HasAnyFunctionFlags(FUNC_Static);
}

#if WITH_EDITOR
EDataValidationResult UMDFastBindingValue_StaticFunction::IsDataValid(TArray<FText>& ValidationErrors)
{
	EDataValidationResult Result = Super::IsDataValid(ValidationErrors);
	
	if (FunctionOwnerClass == nullptr)
	{
		ValidationErrors.Add(LOCTEXT("NullFunctionOwnerClass", "Select a class to pick a function from"));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
