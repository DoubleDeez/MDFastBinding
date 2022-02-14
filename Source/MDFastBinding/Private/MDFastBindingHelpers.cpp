// Fill out your copyright notice in the Description page of Project Settings.


#include "MDFastBindingHelpers.h"

void FMDFastBindingHelpers::GetFunctionParamProps(const UFunction* Func, TArray<const FProperty*>& OutParams)
{
	OutParams.Empty();
	
	if (Func != nullptr)
	{
		for (TFieldIterator<const FProperty> It(Func); It; ++It)
		{
			if (const FProperty* Param = *It)
			{
				if (Param->HasAnyPropertyFlags(CPF_Parm))
				{
					OutParams.Add(Param);
				}
			}
		}
	}
}

void FMDFastBindingHelpers::SplitFunctionParamsAndReturnProp(const UFunction* Func, TArray<const FProperty*>& OutParams,
                                                             const FProperty*& OutReturnProp)
{
	OutParams.Empty();
	OutReturnProp = nullptr;
	
	if (Func != nullptr)
	{
		OutReturnProp = Func->GetReturnProperty();
		
		for (TFieldIterator<const FProperty> It(Func); It; ++It)
		{
			if (const FProperty* Param = *It)
			{
				if (Param->HasAnyPropertyFlags(CPF_Parm))
				{
					const bool bIsReturnParam = Param->HasAnyPropertyFlags(CPF_ReturnParm)
						|| (Param->HasAnyPropertyFlags(CPF_OutParm) && !Param->HasAnyPropertyFlags(CPF_ReferenceParm));
					if (Param != OutReturnProp && !bIsReturnParam)
					{
						OutParams.Add(Param);
					}
					else if (OutReturnProp == nullptr && bIsReturnParam)
					{
						OutReturnProp = Param;
					}
				}
			}
		}
	}
}

FString FMDFastBindingHelpers::PropertyToString(const FProperty& Prop)
{
	if (const FArrayProperty* ArrayProp = CastField<const FArrayProperty>(&Prop))
	{
		return FString::Printf(TEXT("%s<%s>"), *Prop.GetCPPType(), *PropertyToString(*ArrayProp->Inner));
	}
	else if (const FSetProperty* SetProp = CastField<const FSetProperty>(&Prop))
	{
		return FString::Printf(TEXT("%s<%s>"), *Prop.GetCPPType(), *PropertyToString(*SetProp->ElementProp));
	}
	else if (const FMapProperty* MapProp = CastField<const FMapProperty>(&Prop))
	{
		return FString::Printf(TEXT("%s<%s, %s>"), *Prop.GetCPPType(), *PropertyToString(*MapProp->KeyProp), *PropertyToString(*MapProp->ValueProp));
	}

	return Prop.GetCPPType();
}
