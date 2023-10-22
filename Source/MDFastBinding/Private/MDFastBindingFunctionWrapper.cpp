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

	const bool bIsFuncValid = IsValid(FunctionPtr);
#if WITH_EDITORONLY_DATA
	LastFrameFunctionUpdated = bIsFuncValid ? GFrameCounter : 0;
#endif

	return bIsFuncValid;
}

UClass* FMDFastBindingFunctionWrapper::GetFunctionOwnerClass() const
{
	return OwnerClassGetter.IsBound() ? OwnerClassGetter.Execute() : nullptr;
}

const TArray<const FProperty*>& FMDFastBindingFunctionWrapper::GetParams()
{
#if WITH_EDITORONLY_DATA
	if (LastFrameFunctionUpdated != GFrameCounter)
#else
	if (FunctionPtr == nullptr)
#endif
	{
		BuildFunctionData();
	}

	return Params;
}

const FProperty* FMDFastBindingFunctionWrapper::GetReturnProp()
{
#if WITH_EDITORONLY_DATA
	if (LastFrameFunctionUpdated != GFrameCounter || LastFrameFunctionUpdated == 0)
#else
	if (FunctionPtr == nullptr)
#endif
	{
		BuildFunctionData();
	}

	return ReturnProp;
}

UFunction* FMDFastBindingFunctionWrapper::GetFunctionPtr()
{
#if WITH_EDITORONLY_DATA
	if (LastFrameFunctionUpdated != GFrameCounter)
#else
	if (FunctionPtr == nullptr)
#endif
	{
		BuildFunctionData();
	}

	return FunctionPtr;
}

TTuple<const FProperty*, void*> FMDFastBindingFunctionWrapper::CallFunction(UObject* SourceObject)
{
#if WITH_EDITORONLY_DATA
	if (LastFrameFunctionUpdated != GFrameCounter)
#else
	if (FunctionPtr == nullptr)
#endif
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

	if (ReturnProp != nullptr)
	{
		void* ReturnValuePtr = static_cast<uint8*>(FunctionMemory) + ReturnProp->GetOffset_ForUFunction();
		return TTuple<const FProperty*, void*>{ ReturnProp, ReturnValuePtr };
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
	const FProperty* ReturnProp = nullptr;
	TArray<const FProperty*> Params;
	FMDFastBindingHelpers::SplitFunctionParamsAndReturnProp(Func, Params, ReturnProp);

	return FunctionToString_Internal(Func, ReturnProp, Params);
}

FString FMDFastBindingFunctionWrapper::FunctionToString_Internal(UFunction* Func, const FProperty* ReturnProp, const TArray<const FProperty*>& Params)
{
	if (Func == nullptr)
	{
		return TEXT("None");
	}

	FString ParamString;
	for (int32 i = 0; i < Params.Num(); ++i)
	{
		if (const FProperty* Param = Params[i])
		{
			if (i != 0)
			{
				ParamString += TEXT(", ");
			}

			ParamString += FMDFastBindingHelpers::PropertyToString(*Param) + TEXT(" ") + Param->GetName();
		}
	}

	const FString ReturnString = ReturnProp != nullptr ? FMDFastBindingHelpers::PropertyToString(*ReturnProp) : TEXT("void");

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
