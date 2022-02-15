#include "BindingValues/MDFastBindingValue_Function.h"

#include "MDFastBinding.h"
#include "BindingDestinations/MDFastBindingDestinationBase.h"

#define LOCTEXT_NAMESPACE "MDFastBindingDestination_Function"

namespace MDFastBindingValue_Function_Private
{
	const FName FunctionOwnerName = TEXT("Function Owner");
}

TTuple<const FProperty*, void*> UMDFastBindingValue_Function::GetValue(UObject* SourceObject)
{
	return Function.CallFunction(SourceObject);
}

const FProperty* UMDFastBindingValue_Function::GetOutputProperty()
{
	return Function.GetReturnProp();
}

bool UMDFastBindingValue_Function::DoesBindingItemDefaultToSelf(const FName& InItemName) const
{
	return InItemName == MDFastBindingValue_Function_Private::FunctionOwnerName;
}

#if WITH_EDITORONLY_DATA
FText UMDFastBindingValue_Function::GetDisplayName()
{
	if (!DevName.IsEmptyOrWhitespace())
	{
		return DevName;
	}

	if (const UFunction* Func = Function.GetFunctionPtr())
	{
		return Func->GetDisplayNameText();
	}
	
	return Super::GetDisplayName();
}
#endif

UObject* UMDFastBindingValue_Function::GetFunctionOwner(UObject* SourceObject)
{
	const TTuple<const FProperty*, void*> FunctionOwner = GetBindingItemValue(SourceObject, MDFastBindingValue_Function_Private::FunctionOwnerName);
	if (FunctionOwner.Value != nullptr)
	{
		return *static_cast<UObject**>(FunctionOwner.Value);
	}
	
	return SourceObject;
}

UClass* UMDFastBindingValue_Function::GetFunctionOwnerClass()
{
	if (const FObjectPropertyBase* ObjectProp = CastField<const FObjectPropertyBase>(GetBindingItemValueProperty(MDFastBindingValue_Function_Private::FunctionOwnerName)))
	{
		return ObjectProp->PropertyClass;
	}
	
	return GetBindingOuterClass();
}

void UMDFastBindingValue_Function::PopulateFunctionParam(UObject* SourceObject, const FProperty* Param, void* ValuePtr)
{
	if (Param == nullptr || ValuePtr == nullptr)
	{
		return;
	}
	
	const TTuple<const FProperty*, void*> ParamValue = GetBindingItemValue(SourceObject, Param->GetFName());
	FMDFastBindingModule::SetProperty(Param, ValuePtr, ParamValue.Key, ParamValue.Value);
}

bool UMDFastBindingValue_Function::IsFunctionValid(UFunction* Func, const FProperty* ReturnValue, const TArray<const FProperty*>& Params) const
{
	return ReturnValue != nullptr;
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
			if (UMDFastBindingDestinationBase* OuterDest = GetOuterBindingDestination())
			if (BindingItems[i].Value != nullptr)
			{
				OuterDest->AddOrphan(BindingItems[i].Value);
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
		EnsureBindingItemExists(Param->GetFName(), Param, Param->GetToolTipText());
	}
}

void UMDFastBindingValue_Function::PostInitProperties()
{
	Function.OwnerClassGetter.BindUObject(this, &UMDFastBindingValue_Function::GetFunctionOwnerClass);
	Function.OwnerGetter.BindUObject(this, &UMDFastBindingValue_Function::GetFunctionOwner);
	Function.ParamPopulator.BindUObject(this, &UMDFastBindingValue_Function::PopulateFunctionParam);
	Function.FunctionFilter.BindUObject(this, &UMDFastBindingValue_Function::IsFunctionValid);
	
	Super::PostInitProperties();
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
#endif

#undef LOCTEXT_NAMESPACE
