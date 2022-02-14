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
	if (const UClass* OwnerClass = GetFunctionOwnerClass())
	{
		FunctionPtr = OwnerClass->FindFunctionByName(FunctionName);
		if (FunctionPtr != nullptr)
		{
			FMDFastBindingHelpers::SplitFunctionParamsAndReturnProp(FunctionPtr, Params, ReturnProp);
		}
	}

	return FunctionPtr != nullptr;
}

UClass* FMDFastBindingFunctionWrapper::GetFunctionOwnerClass() const
{
	return OwnerClassGetter.IsBound() ? OwnerClassGetter.Execute() : nullptr;
}

const TArray<const FProperty*>& FMDFastBindingFunctionWrapper::GetParams()
{
#if !WITH_EDITOR
	if (FunctionPtr == nullptr)
#endif
	{
		BuildFunctionData();
	}
	
	return Params;
}

const FProperty* FMDFastBindingFunctionWrapper::GetReturnProp()
{
#if !WITH_EDITOR
	if (FunctionPtr == nullptr)
#endif
	{
		BuildFunctionData();
	}
	
	return ReturnProp;
}

UFunction* FMDFastBindingFunctionWrapper::GetFunctionPtr()
{
#if !WITH_EDITOR
	if (FunctionPtr == nullptr)
#endif
	{
		BuildFunctionData();
	}
	
	return FunctionPtr;
}

TTuple<const FProperty*, void*> FMDFastBindingFunctionWrapper::CallFunction(UObject* SourceObject)
{
#if !WITH_EDITOR
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

	FunctionOwner->ProcessEvent(FunctionPtr, FunctionMemory);

	if (ReturnProp != nullptr)
	{
		void* ReturnValuePtr = static_cast<uint8*>(FunctionMemory) + ReturnProp->GetOffset_ForUFunction();
		return TTuple<const FProperty*, void*>{ ReturnProp, ReturnValuePtr };
	}

	return {};
}

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

bool FMDFastBindingFunctionWrapper::IsFunctionValidForWrapper(const UFunction* Func)
{
	return Func != nullptr && Func->HasAnyFunctionFlags(FUNC_BlueprintCallable);
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

	return FString::Printf(TEXT("%s %s(%s)"), *ReturnString, *Func->GetDisplayNameText().ToString(), *ParamString);
}
