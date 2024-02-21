#include "BindingValues/MDFastBindingValue_Function.h"

#include "MDFastBinding.h"
#include "BindingDestinations/MDFastBindingDestinationBase.h"
#include "Utils/MDFastBindingTraceHelpers.h"

#define LOCTEXT_NAMESPACE "MDFastBindingDestination_Function"

namespace MDFastBindingValue_Function_Private
{
	const FName FunctionOwnerName = TEXT("Function Owner");
}

UMDFastBindingValue_Function::UMDFastBindingValue_Function()
{
	// Make the default case more 'correct' since a function _could_ return different results given the same inputs
	UpdateType = EMDFastBindingUpdateType::Always;
}

TTuple<const FProperty*, void*> UMDFastBindingValue_Function::GetValue_Internal(UObject* SourceObject)
{
#if defined(MDFASTBINDING_CONDENSED_PROFILING) && MDFASTBINDING_CONDENSED_PROFILING
	MD_TRACE_CPUPROFILER_EVENT_SCOPE_FUNCTION_TEXT(*Function.GetFunctionName().ToString());
#else
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(__FUNCTION__);
	TRACE_CPUPROFILER_EVENT_SCOPE_TEXT(*Function.GetFunctionName().ToString());
#endif
	
	bNeedsUpdate = false;
	return Function.CallFunction(SourceObject);
}

const FProperty* UMDFastBindingValue_Function::GetOutputProperty()
{
	return Function.GetReturnProp();
}

#if WITH_EDITORONLY_DATA
bool UMDFastBindingValue_Function::DoesBindingItemDefaultToSelf(const FName& InItemName) const
{
	const UFunction* Func = const_cast<UMDFastBindingValue_Function*>(this)->GetFunction();
	const FName DefaultToSelfMetaValue = IsValid(Func) ? *Func->GetMetaData(TEXT("DefaultToSelf")) : TEXT("");

	return InItemName == DefaultToSelfMetaValue || InItemName == MDFastBindingValue_Function_Private::FunctionOwnerName;
}

bool UMDFastBindingValue_Function::IsBindingItemWorldContextObject(const FName& InItemName) const
{
	const UFunction* Func = const_cast<UMDFastBindingValue_Function*>(this)->GetFunction();
	const FName WorldContextMetaValue = IsValid(Func) ? *Func->GetMetaData(TEXT("WorldContext")) : TEXT("");

	return InItemName == WorldContextMetaValue;
}

FText UMDFastBindingValue_Function::GetDisplayName()
{
	if (const UFunction* Func = Function.GetFunctionPtr())
	{
		return Func->GetDisplayNameText();
	}

	return Super::GetDisplayName();
}
#endif

UObject* UMDFastBindingValue_Function::GetFunctionOwner(UObject* SourceObject)
{
	bool bDidUpdate = false;
	const TTuple<const FProperty*, void*> FunctionOwner = GetBindingItemValue(SourceObject, MDFastBindingValue_Function_Private::FunctionOwnerName, bDidUpdate);
	bNeedsUpdate |= bDidUpdate;

	if (FunctionOwner.Value != nullptr)
	{
		return *static_cast<UObject**>(FunctionOwner.Value);
	}
	else if (FunctionOwner.Key != nullptr)
	{
		// null value, but key is valid so it failed to get a value, just return null as the owner
		return nullptr;
	}

	return SourceObject;
}

UClass* UMDFastBindingValue_Function::GetFunctionOwnerClass()
{
	if (const FObjectPropertyBase* ObjectProp = CastField<const FObjectPropertyBase>(GetBindingItemValueProperty(MDFastBindingValue_Function_Private::FunctionOwnerName)))
	{
		return ObjectProp->PropertyClass;
	}

	return GetBindingOwnerClass();
}

void UMDFastBindingValue_Function::PopulateFunctionParam(UObject* SourceObject, const FProperty* Param, void* ValuePtr)
{
	if (Param == nullptr || ValuePtr == nullptr)
	{
		return;
	}

	bool bDidUpdate = false;
	const TTuple<const FProperty*, void*> ParamValue = GetBindingItemValue(SourceObject, Param->GetFName(), bDidUpdate);
	FMDFastBindingModule::SetPropertyDirectly(Param, ValuePtr, ParamValue.Key, ParamValue.Value);
	bNeedsUpdate |= bDidUpdate;
}

bool UMDFastBindingValue_Function::IsFunctionValid(UFunction* Func, const TWeakFieldPtr<const FProperty>& ReturnValue, const TArray<TWeakFieldPtr<const FProperty>>& Params) const
{
	return ReturnValue.IsValid();
}

void UMDFastBindingValue_Function::SetupBindingItems()
{
	Super::SetupBindingItems();

	const TArray<const FProperty*>& Params = Function.GetParams();
	TSet<FName> ExpectedInputs;
	if (bAddPathRootBindingItem)
	{
		ExpectedInputs.Add(MDFastBindingValue_Function_Private::FunctionOwnerName);
	}

	for (const FProperty* Param : Params)
	{
		ExpectedInputs.Add(Param->GetFName());
	}

	for (int32 i = BindingItems.Num() - 1; i >= 0; --i)
	{
		if (!ExpectedInputs.Contains(BindingItems[i].ItemName))
		{
#if WITH_EDITORONLY_DATA
			if (BindingItems[i].Value != nullptr)
			{
				OrphanBindingItem(BindingItems[i].Value);
			}
#endif
			BindingItems.RemoveAt(i);
		}
	}

	if (bAddPathRootBindingItem)
	{
		EnsureBindingItemExists(MDFastBindingValue_Function_Private::FunctionOwnerName
			, GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UMDFastBindingValue_Function, ObjectProperty))
			, LOCTEXT("PathRootToolTip", "The root object that has the function to call. (Defaults to 'Self').")
			, true);
	}

	for (const FProperty* Param : Params)
	{
#if WITH_EDITORONLY_DATA
		EnsureBindingItemExists(Param->GetFName(), Param, Param->GetToolTipText());
#else
		EnsureBindingItemExists(Param->GetFName(), Param, FText::GetEmpty());
#endif
	}
}

void UMDFastBindingValue_Function::PostInitProperties()
{
	Function.OwnerClassGetter.BindUObject(this, &UMDFastBindingValue_Function::GetFunctionOwnerClass);
	Function.OwnerGetter.BindUObject(this, &UMDFastBindingValue_Function::GetFunctionOwner);
	Function.ParamPopulator.BindUObject(this, &UMDFastBindingValue_Function::PopulateFunctionParam);
	Function.FunctionFilter.BindUObject(this, &UMDFastBindingValue_Function::IsFunctionValid);
	Function.ShouldCallFunction.BindUObject(this, &UMDFastBindingValue_Function::ShouldCallFunction);

	Super::PostInitProperties();
}

bool UMDFastBindingValue_Function::ShouldCallFunction()
{
	return UpdateType != EMDFastBindingUpdateType::IfUpdatesNeeded || bNeedsUpdate;
}

#if WITH_EDITOR
EDataValidationResult UMDFastBindingValue_Function::IsDataValid(TArray<FText>& ValidationErrors)
{
	EDataValidationResult Result = Super::IsDataValid(ValidationErrors);

	if (Function.GetFunctionName() == NAME_None)
	{
		ValidationErrors.Add(LOCTEXT("EmptyFunctionName", "Select a function to call"));
		Result = EDataValidationResult::Invalid;
	}
	else if (!Function.BuildFunctionData())
	{
		ValidationErrors.Add(FText::Format(LOCTEXT("InvalidFunctionData", "Could not find the function [{0}]"), FText::FromName(Function.GetFunctionName())));
		Result = EDataValidationResult::Invalid;
	}
	else if (Function.GetReturnProp() == nullptr)
	{
		ValidationErrors.Add(FText::Format(LOCTEXT("NoReturnValue", "The function [{0}] does not have a return value"), FText::FromName(Function.GetFunctionName())));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}

void UMDFastBindingValue_Function::OnVariableRenamed(UClass* VariableClass, const FName& OldVariableName, const FName& NewVariableName)
{
	Super::OnVariableRenamed(VariableClass, OldVariableName, NewVariableName);

	Function.OnVariableRenamed(VariableClass, OldVariableName, NewVariableName);
}

UFunction* UMDFastBindingValue_Function::GetFunction()
{
	return Function.GetFunctionPtr();
}
#endif

#undef LOCTEXT_NAMESPACE
