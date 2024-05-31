#include "BindingDestinations/MDFastBindingDestination_Function.h"

#include "MDFastBinding.h"
#include "Utils/MDFastBindingTraceHelpers.h"

#define LOCTEXT_NAMESPACE "MDFastBindingDestination_Function"

namespace MDFastBindingDestination_Function_Private
{
	const FName FunctionOwnerName = TEXT("Function Owner");
}

void UMDFastBindingDestination_Function::InitializeDestination_Internal(UObject* SourceObject)
{
	Super::InitializeDestination_Internal(SourceObject);

	Function.BuildFunctionData();
}

void UMDFastBindingDestination_Function::UpdateDestination_Internal(UObject* SourceObject)
{
#if defined(MDFASTBINDING_CONDENSED_PROFILING) && MDFASTBINDING_CONDENSED_PROFILING
	MD_TRACE_CPUPROFILER_EVENT_SCOPE_FUNCTION_TEXT(*Function.GetFunctionName().ToString());
#else
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(__FUNCTION__);
	TRACE_CPUPROFILER_EVENT_SCOPE_TEXT(*Function.GetFunctionName().ToString());
#endif
	
	bNeedsUpdate = false;
	Function.CallFunction(SourceObject);
}

UObject* UMDFastBindingDestination_Function::GetFunctionOwner(UObject* SourceObject)
{
	bool bDidUpdate = false;
	const TTuple<const FProperty*, void*> FunctionOwner = GetBindingItemValue(SourceObject, MDFastBindingDestination_Function_Private::FunctionOwnerName, bDidUpdate);
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

UClass* UMDFastBindingDestination_Function::GetFunctionOwnerClass()
{
	if (const FObjectPropertyBase* ObjectProp = CastField<const FObjectPropertyBase>(GetBindingItemValueProperty(MDFastBindingDestination_Function_Private::FunctionOwnerName)))
	{
		return ObjectProp->PropertyClass;
	}

	return GetBindingOwnerClass();
}

void UMDFastBindingDestination_Function::PopulateFunctionParam(UObject* SourceObject, const FProperty* Param, void* ValuePtr)
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

void UMDFastBindingDestination_Function::SetupBindingItems()
{
	Super::SetupBindingItems();

	const TArray<const FProperty*>& Params = Function.GetParams();
	TSet<FName> ExpectedInputs = { MDFastBindingDestination_Function_Private::FunctionOwnerName };

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

	EnsureBindingItemExists(MDFastBindingDestination_Function_Private::FunctionOwnerName
		, GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UMDFastBindingDestination_Function, ObjectProperty))
		, LOCTEXT("PathRootToolTip", "The root object that has the function to call. (Defaults to 'Self').")
		, true);


	for (const FProperty* Param : Params)
	{
		if (Param != nullptr)
		{
#if WITH_EDITORONLY_DATA
			EnsureBindingItemExists(Param->GetFName(), Param, Param->GetToolTipText());
#else
			EnsureBindingItemExists(Param->GetFName(), Param, FText::GetEmpty());
#endif
		}
	}
}

void UMDFastBindingDestination_Function::PostInitProperties()
{
	Function.OwnerClassGetter.BindUObject(this, &UMDFastBindingDestination_Function::GetFunctionOwnerClass);
	Function.OwnerGetter.BindUObject(this, &UMDFastBindingDestination_Function::GetFunctionOwner);
	Function.ParamPopulator.BindUObject(this, &UMDFastBindingDestination_Function::PopulateFunctionParam);
	Function.ShouldCallFunction.BindUObject(this, &UMDFastBindingDestination_Function::ShouldCallFunction);

	Super::PostInitProperties();
}

bool UMDFastBindingDestination_Function::ShouldCallFunction()
{
	const bool bResult = UpdateType != EMDFastBindingUpdateType::IfUpdatesNeeded || bNeedsUpdate;

	if (bResult)
	{
		MarkAsHasEverUpdated();
	}

	return bResult;
}

#if WITH_EDITORONLY_DATA
bool UMDFastBindingDestination_Function::DoesBindingItemDefaultToSelf(const FName& InItemName) const
{
	const UFunction* Func = const_cast<UMDFastBindingDestination_Function*>(this)->GetFunction();
	const FName DefaultToSelfMetaValue = IsValid(Func) ? *Func->GetMetaData(TEXT("DefaultToSelf")) : TEXT("");

	return InItemName == DefaultToSelfMetaValue || InItemName == MDFastBindingDestination_Function_Private::FunctionOwnerName;
}

bool UMDFastBindingDestination_Function::IsBindingItemWorldContextObject(const FName& InItemName) const
{
	const UFunction* Func = const_cast<UMDFastBindingDestination_Function*>(this)->GetFunction();
	const FName WorldContextMetaValue = IsValid(Func) ? *Func->GetMetaData(TEXT("WorldContext")) : TEXT("");

	return InItemName == WorldContextMetaValue;
}
#endif

#if WITH_EDITOR
EDataValidationResult UMDFastBindingDestination_Function::IsDataValid(TArray<FText>& ValidationErrors)
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

	return Result;
}

void UMDFastBindingDestination_Function::OnVariableRenamed(UClass* VariableClass, const FName& OldVariableName, const FName& NewVariableName)
{
	Super::OnVariableRenamed(VariableClass, OldVariableName, NewVariableName);

	Function.OnVariableRenamed(VariableClass, OldVariableName, NewVariableName);
}

UFunction* UMDFastBindingDestination_Function::GetFunction()
{
	return Function.GetFunctionPtr();
}

void UMDFastBindingDestination_Function::SetFunction(UFunction* Func)
{
	Function.FunctionMember.bIsFunction = true;
	Function.FunctionMember.SetFromField<UFunction>(Func, false);
}
#endif

#if WITH_EDITORONLY_DATA
FText UMDFastBindingDestination_Function::GetDisplayName()
{
	if (const UFunction* Func = Function.GetFunctionPtr())
	{
		return Func->GetDisplayNameText();
	}

	return Super::GetDisplayName();
}
#endif

#undef LOCTEXT_NAMESPACE
