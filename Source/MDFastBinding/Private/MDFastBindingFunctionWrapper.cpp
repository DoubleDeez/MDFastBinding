#include "MDFastBindingFunctionWrapper.h"

#include "MDFastBindingHelpers.h"


FMDFastBindingFunctionWrapper::~FMDFastBindingFunctionWrapper()
{
	if (FunctionMemory != nullptr)
	{
		FMemory::Free(FunctionMemory);
		FunctionMemory = nullptr;
	}
}

bool FMDFastBindingFunctionWrapper::BuildFunctionData()
{
	FixupFunctionMember();

	FunctionPtr = FunctionMember.ResolveMember<UFunction>();
	if (FunctionPtr != nullptr)
	{
		FMDFastBindingHelpers::SplitFunctionParamsAndReturnProp(FunctionPtr, Params, ReturnProp);
	}

	RefreshCachedProperties();

	const bool bIsFuncValid = IsValid(FunctionPtr);
#if WITH_EDITORONLY_DATA
	LastFrameFunctionUpdated = bIsFuncValid ? GFrameCounter : TOptional<uint64>();
#endif

	return bIsFuncValid;
}

UClass* FMDFastBindingFunctionWrapper::GetFunctionOwnerClass() const
{
	return OwnerClassGetter.IsBound() ? OwnerClassGetter.Execute() : nullptr;
}

TArray<const FProperty*> FMDFastBindingFunctionWrapper::GetParams()
{
	if (ShouldRebuildFunctionData())
	{
		BuildFunctionData();
	}

	return CachedParams;
}

const FProperty* FMDFastBindingFunctionWrapper::GetReturnProp()
{
	if (ShouldRebuildFunctionData())
	{
		BuildFunctionData();
	}

	return CachedReturnProp;
}

UFunction* FMDFastBindingFunctionWrapper::GetFunctionPtr()
{
	if (ShouldRebuildFunctionData())
	{
		BuildFunctionData();
	}

	return FunctionPtr;
}

TTuple<const FProperty*, void*> FMDFastBindingFunctionWrapper::CallFunction(UObject* SourceObject)
{
	if (ShouldRebuildFunctionData())
	{
		BuildFunctionData();
	}

	InitFunctionMemory();

	UObject* FunctionOwner = GetFunctionOwner(SourceObject);
	if (SourceObject == nullptr || FunctionPtr == nullptr || FunctionOwner == nullptr || FunctionMemory == nullptr)
	{
		return {};
	}

	PopulateParams(SourceObject);

	if (ShouldCallFunction.IsBound() && !ShouldCallFunction.Execute())
	{
		return {};
	}

	FunctionOwner->ProcessEvent(FunctionPtr, FunctionMemory);

	if (CachedReturnProp != nullptr)
	{
		void* ReturnValuePtr = static_cast<uint8*>(FunctionMemory) + CachedReturnProp->GetOffset_ForUFunction();
		return TTuple<const FProperty*, void*>{ CachedReturnProp, ReturnValuePtr };
	}

	return {};
}

#if WITH_EDITORONLY_DATA
FString FMDFastBindingFunctionWrapper::ToString()
{
	BuildFunctionData();

	return FunctionToString_Internal(FunctionPtr, ReturnProp, Params);
}

FString FMDFastBindingFunctionWrapper::FunctionToString(UFunction* Func)
{
	TWeakFieldPtr<const FProperty> ReturnProp = nullptr;
	TArray<TWeakFieldPtr<const FProperty>> Params;
	FMDFastBindingHelpers::SplitFunctionParamsAndReturnProp(Func, Params, ReturnProp);

	return FunctionToString_Internal(Func, ReturnProp, Params);
}

FString FMDFastBindingFunctionWrapper::FunctionToString_Internal(UFunction* Func, const TWeakFieldPtr<const FProperty>& ReturnProp, const TArray<TWeakFieldPtr<const FProperty>>& Params)
{
	if (Func == nullptr)
	{
		return TEXT("None");
	}

	FString ParamString;
	for (int32 i = 0; i < Params.Num(); ++i)
	{
		if (const FProperty* Param = Params[i].Get())
		{
			if (i != 0)
			{
				ParamString += TEXT(", ");
			}

			ParamString += FMDFastBindingHelpers::PropertyToString(*Param) + TEXT(" ") + Param->GetName();
		}
	}

	const FProperty* ReturnPropPtr = ReturnProp.Get(); 
	const FString ReturnString = ReturnPropPtr != nullptr ? FMDFastBindingHelpers::PropertyToString(*ReturnPropPtr) : TEXT("void");

	return FString::Printf(TEXT("%s %s(%s)"), *ReturnString, *Func->GetFName().ToString(), *ParamString);
}
#endif

#if WITH_EDITOR
void FMDFastBindingFunctionWrapper::OnVariableRenamed(UClass* VariableClass, const FName& OldVariableName, const FName& NewVariableName)
{
	if (FunctionMember.GetMemberName() == OldVariableName)
	{
		const UClass* OwnerClass = GetFunctionOwnerClass();
		if (OwnerClass != nullptr && OwnerClass == VariableClass)
		{
			FunctionMember.SetMemberName(NewVariableName);
		}

		BuildFunctionData();
	}
}
#endif

bool FMDFastBindingFunctionWrapper::ShouldRebuildFunctionData() const
{
#if WITH_EDITORONLY_DATA
	return !LastFrameFunctionUpdated.IsSet()
	|| LastFrameFunctionUpdated.GetValue() != GFrameCounter
	|| FunctionMember.ResolveMember<UFunction>() != FunctionPtr;
#else
	return FunctionPtr == nullptr;
#endif
}

UObject* FMDFastBindingFunctionWrapper::GetFunctionOwner(UObject* SourceObject) const
{
	return OwnerGetter.IsBound() ? OwnerGetter.Execute(SourceObject) : nullptr;
}

void FMDFastBindingFunctionWrapper::InitFunctionMemory()
{
	if (FunctionPtr != nullptr && FunctionMemory == nullptr)
	{
		TArray<const FProperty*> AllParams;
		FMDFastBindingHelpers::GetFunctionParamProps(FunctionPtr, AllParams);

		for (const FProperty* Param : AllParams)
		{
			if (FunctionMemory == nullptr)
			{
				FunctionMemory = FMemory::Malloc(FunctionPtr->ParmsSize, Param->GetMinAlignment());
			}

			Param->InitializeValue_InContainer(FunctionMemory);
		}
	}
}

void FMDFastBindingFunctionWrapper::PopulateParams(UObject* SourceObject)
{
	if (FunctionMemory != nullptr && ParamPopulator.IsBound())
	{
		for (const FProperty* Param : GetParams())
		{
			ParamPopulator.Execute(SourceObject, Param, static_cast<uint8*>(FunctionMemory) + Param->GetOffset_ForUFunction());
		}
	}
}

void FMDFastBindingFunctionWrapper::FixupFunctionMember()
{
	if (UClass* OwnerClass = GetFunctionOwnerClass())
	{
		if (FunctionMember.GetMemberName() == NAME_None && FunctionName != NAME_None)
		{
			if (UFunction* Func = OwnerClass->FindFunctionByName(FunctionName))
			{
				FunctionName = NAME_None;
				FunctionMember.SetFromField<UFunction>(Func, false);
				FunctionMember.bIsFunction = true;
			}
		}

		// Check if FunctionMember needs to be updated with a new owner, likely due to a reparented or duplicated BP
		FunctionMember.FixUpReference(*OwnerClass);
	}
}

void FMDFastBindingFunctionWrapper::RefreshCachedProperties()
{
	CachedParams.Reset(Params.Num());
	
	for (const TWeakFieldPtr<const FProperty>& WeakParam : Params)
	{
		if (const FProperty* Param = WeakParam.Get())
		{
			CachedParams.Add(Param);
		}
	}

	CachedReturnProp = ReturnProp.Get();
}
