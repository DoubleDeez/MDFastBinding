#include "BindingValues/MDFastBindingValue_StaticFunction.h"

#include "MDFastBindingHelpers.h"

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
	return Super::IsFunctionValid(Func, ReturnValue, Params) && Func != nullptr && Func->HasAllFunctionFlags(FUNC_Static | FUNC_BlueprintCallable | FUNC_Public);
}

bool UMDFastBindingValue_StaticFunction::DoesClassHaveValidStaticFunctions(const UClass* InClass) const
{
	if (IsValid(InClass))
	{
		for (TFieldIterator<UFunction> It(InClass); It; ++It)
		{
			TWeakFieldPtr<const FProperty> ReturnProp = nullptr;
			TArray<TWeakFieldPtr<const FProperty>> Params;
			FMDFastBindingHelpers::SplitFunctionParamsAndReturnProp(*It, Params, ReturnProp);

			if (IsFunctionValid(*It, ReturnProp, Params))
			{
				return true;
			}
		}
	}

	return false;
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
