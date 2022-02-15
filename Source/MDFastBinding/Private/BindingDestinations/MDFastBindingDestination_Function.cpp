#include "BindingDestinations/MDFastBindingDestination_Function.h"

#include "MDFastBinding.h"

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
	Function.CallFunction(SourceObject);
}

UObject* UMDFastBindingDestination_Function::GetFunctionOwner(UObject* SourceObject)
{
	const TTuple<const FProperty*, void*> FunctionOwner = GetBindingItemValue(SourceObject, MDFastBindingDestination_Function_Private::FunctionOwnerName);
	if (FunctionOwner.Value != nullptr)
	{
		if (UObject* Owner = *static_cast<UObject**>(FunctionOwner.Value))
		{
			return Owner;
		}
	}

	return SourceObject;
}

UClass* UMDFastBindingDestination_Function::GetFunctionOwnerClass()
{
	if (const FObjectPropertyBase* ObjectProp = CastField<const FObjectPropertyBase>(GetBindingItemValueProperty(MDFastBindingDestination_Function_Private::FunctionOwnerName)))
	{
		return ObjectProp->PropertyClass;
	}
	
	return GetBindingOuterClass();
}

void UMDFastBindingDestination_Function::PopulateFunctionParam(UObject* SourceObject, const FProperty* Param, void* ValuePtr)
{
	if (Param == nullptr || ValuePtr == nullptr)
	{
		return;
	}
	
	const TTuple<const FProperty*, void*> ParamValue = GetBindingItemValue(SourceObject, Param->GetFName());
	FMDFastBindingModule::SetProperty(Param, ValuePtr, ParamValue.Key, ParamValue.Value);
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
				AddOrphan(BindingItems[i].Value);
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
		EnsureBindingItemExists(Param->GetFName(), Param, Param->GetToolTipText());
	}
}

void UMDFastBindingDestination_Function::PostInitProperties()
{
	Function.OwnerClassGetter.BindUObject(this, &UMDFastBindingDestination_Function::GetFunctionOwnerClass);
	Function.OwnerGetter.BindUObject(this, &UMDFastBindingDestination_Function::GetFunctionOwner);
	Function.ParamPopulator.BindUObject(this, &UMDFastBindingDestination_Function::PopulateFunctionParam);
	
	Super::PostInitProperties();
}

bool UMDFastBindingDestination_Function::DoesBindingItemDefaultToSelf(const FName& InItemName) const
{
	return InItemName == MDFastBindingDestination_Function_Private::FunctionOwnerName;
}

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
#endif

#undef LOCTEXT_NAMESPACE
