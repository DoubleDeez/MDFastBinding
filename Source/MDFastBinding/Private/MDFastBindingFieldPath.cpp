#include "MDFastBindingFieldPath.h"

#include "MDFastBindingHelpers.h"

FMDFastBindingFieldPath::~FMDFastBindingFieldPath()
{
	CleanupFunctionMemory();
}

bool FMDFastBindingFieldPath::BuildPath()
{
	FixupFieldPath();
	
	CachedPath.Empty(FieldPathMembers.Num());

	for (const FMDFastBindingMemberReference& FieldPathMember : FieldPathMembers)
	{
		if (FieldPathMember.bIsFunction)
		{
			CachedPath.Add(FieldPathMember.ResolveMember<UFunction>());
		}
		else
		{
			CachedPath.Add(FieldPathMember.ResolveMember<FProperty>());
		}
	}
	
	return CachedPath.Num() > 0 && CachedPath.Num() == FieldPathMembers.Num();
}

const TArray<FFieldVariant>& FMDFastBindingFieldPath::GetFieldPath()
{
#if !WITH_EDITOR
	// No caching in editor, since the user could change the path
	if (CachedPath.Num() == 0)
#endif
	{
		BuildPath();
	}

	return CachedPath;
}

TTuple<const FProperty*, void*> FMDFastBindingFieldPath::ResolvePath(UObject* SourceObject)
{
	if (void* Owner = GetPathOwner(SourceObject))
	{
		bool bIsOwnerAUObject = true;
		const TArray<FFieldVariant>& Path = GetFieldPath();
		for (int32 i = 0; i < Path.Num() && Owner != nullptr; ++i)
		{
			const FFieldVariant& FieldVariant = Path[i];
			const FProperty* OwnerProp = nullptr;

			if (UFunction* Func = Cast<UFunction>(FieldVariant.ToUObject()))
			{
				InitFunctionMemory(Func);
				void* FuncMemory = FunctionMemory.FindRef(Func);
				if (!bIsOwnerAUObject || FuncMemory == nullptr)
				{
					return {};
				}

				UObject* OwnerUObject = *static_cast<UObject**>(Owner);
				if (OwnerUObject == nullptr)
				{
					return {};
				}
				
				OwnerUObject->ProcessEvent(Func, FuncMemory);
				
				TArray<const FProperty*> Params;
				FMDFastBindingHelpers::SplitFunctionParamsAndReturnProp(Func, Params, OwnerProp);
				Owner = FuncMemory;
			}
			else if (const FProperty* Prop = CastField<const FProperty>(FieldVariant.ToField()))
			{
				OwnerProp = Prop;

				if (bIsOwnerAUObject)
				{
					if (UObject* OwnerObject = *static_cast<UObject**>(Owner))
					{
						Owner = Prop->ContainerPtrToValuePtr<void>(OwnerObject);
					}
					else
					{
						return {};
					}
				}
				else
				{
					Owner = Prop->ContainerPtrToValuePtr<void>(Owner);
				}
			}
			else
			{
				return {};
			}
			
			const bool bIsEndOfPath = i == (FieldPathMembers.Num() - 1);
			if (bIsEndOfPath)
			{
				return TTuple<const FProperty*, void*>{ OwnerProp, Owner };
			}

			bIsOwnerAUObject = OwnerProp != nullptr && OwnerProp->IsA(FObjectPropertyBase::StaticClass());
		}
	}

	return {};
}

const FProperty* FMDFastBindingFieldPath::GetLeafProperty()
{
	const TArray<FFieldVariant>& Path = GetFieldPath();

	if (Path.Num() > 0)
	{
		const FFieldVariant& FieldVariant = Path.Last();

		if (const UFunction* Func = Cast<UFunction>(FieldVariant.ToUObject()))
		{
			const FProperty* ReturnProp = nullptr;
			TArray<const FProperty*> Params;
			FMDFastBindingHelpers::SplitFunctionParamsAndReturnProp(Func, Params, ReturnProp);
			return ReturnProp;
		}
		else if (const FProperty* Prop = CastField<const FProperty>(FieldVariant.ToField()))
		{
			return Prop;
		}
	}

	return nullptr;
}

bool FMDFastBindingFieldPath::IsLeafFunction()
{
	const TArray<FFieldVariant>& Path = GetFieldPath();

	if (Path.Num() > 0)
	{
		if (Cast<UFunction>(Path.Last().ToUObject()) != nullptr)
		{
			return true;
		}
	}

	return false;
}

bool FMDFastBindingFieldPath::IsPropertyValidForPath(const FProperty& Prop) const
{
	return Prop.HasAnyPropertyFlags(CPF_BlueprintVisible) && (!bOnlyAllowBlueprintReadWriteProperties || !Prop.HasAnyPropertyFlags(CPF_BlueprintReadOnly));
}

bool FMDFastBindingFieldPath::IsFunctionValidForPath(const UFunction& Func)
{
	if (!Func.HasAnyFunctionFlags(FUNC_Const | FUNC_BlueprintPure))
	{
		return false;
	}

	TArray<const FProperty*> Params;
	const FProperty* ReturnProp = nullptr;
	FMDFastBindingHelpers::SplitFunctionParamsAndReturnProp(&Func, Params, ReturnProp);

	return Params.Num() == 0 && ReturnProp != nullptr;
}

FString FMDFastBindingFieldPath::ToString() const
{
	if (FieldPathMembers.Num() == 0)
	{
		return {};
	}

	// Resolving members will fixup any renamed properties
	for (const FMDFastBindingMemberReference& FieldPathMember : FieldPathMembers)
	{
		if (FieldPathMember.bIsFunction)
		{
			FieldPathMember.ResolveMember<UFunction>();
		}
		else
		{
			FieldPathMember.ResolveMember<FProperty>();
		}
	}
	
	FString Result = FieldPathMembers[0].GetMemberName().ToString();
	for (int32 i = 1; i < FieldPathMembers.Num(); ++i)
	{
		Result += FString::Printf(TEXT(".%s"), *(FieldPathMembers[i].GetMemberName().ToString()));
	}

	return Result;
}

#if WITH_EDITOR
void FMDFastBindingFieldPath::OnVariableRenamed(UClass* VariableClass, const FName& OldVariableName, const FName& NewVariableName)
{
	// Since we're only notified for renames of our own variables, we only need to check the first item in the path
	if (FieldPathMembers.Num() > 0 && FieldPathMembers[0].GetMemberName() == OldVariableName)
	{
		const UClass* OwnerClass = GetPathOwnerClass();
		if (OwnerClass != nullptr && OwnerClass == VariableClass)
		{
			FieldPathMembers[0].SetMemberName(NewVariableName);
		}

		BuildPath();
	}
}
#endif

UClass* FMDFastBindingFieldPath::GetPathOwnerClass() const
{
	return OwnerClassGetter.IsBound() ? OwnerClassGetter.Execute() : nullptr;
}

void* FMDFastBindingFieldPath::GetPathOwner(UObject* SourceObject) const
{
	CachedOwnerObject = OwnerGetter.IsBound() ? OwnerGetter.Execute(SourceObject) : nullptr;

	if (CachedOwnerObject != nullptr)
	{
		return &CachedOwnerObject;
	}
	
	return nullptr;
}

void FMDFastBindingFieldPath::InitFunctionMemory(const UFunction* Func)
{
	if (Func != nullptr && !FunctionMemory.Contains(Func))
	{
		void* Memory = nullptr;
		TArray<const FProperty*> Params;
		FMDFastBindingHelpers::GetFunctionParamProps(Func, Params);
		
		for (const FProperty* Param : Params)
		{
			if (Memory == nullptr)
			{
				Memory = FMemory::Malloc(Func->ParmsSize, Param->GetMinAlignment());
				FunctionMemory.Add(Func, Memory);
			}

			Param->InitializeValue_InContainer(Memory);
		}
	}
}

void FMDFastBindingFieldPath::CleanupFunctionMemory()
{
	for(const TPair<TWeakObjectPtr<const UFunction>, void*>& FuncPair : FunctionMemory)
	{
		if (FuncPair.Value != nullptr)
		{
			FMemory::Free(FuncPair.Value);
		}
	}

	FunctionMemory.Empty();
}

void FMDFastBindingFieldPath::FixupFieldPath()
{
	// Check if we need to convert to the Member Reference path
	if (FieldPathMembers.Num() == 0 && FieldPath.Num() != 0)
	{
		if (const UStruct* OwnerStruct = GetPathOwnerClass())
		{
			for (int32 i = 0; i < FieldPath.Num() && OwnerStruct != nullptr; ++i)
			{
				const FName& FieldName = FieldPath[i];

				const FProperty* NextProp = nullptr;
				if (const FProperty* Prop = OwnerStruct->FindPropertyByName(FieldName))
				{
					if (IsPropertyValidForPath(*Prop))
					{
						FMDFastBindingMemberReference& MemberRef = FieldPathMembers.AddDefaulted_GetRef();
						MemberRef.bIsFunction = false;
						MemberRef.SetFromField<FProperty>(Prop, false);
						NextProp = Prop;
					}
					else
					{
						FieldPathMembers.Empty();
						return;
					}
				}
				else if (const UClass* OwnerClass = Cast<const UClass>(OwnerStruct))
				{
					if (const UFunction* Func = OwnerClass->FindFunctionByName(FieldName))
					{
						if (IsFunctionValidForPath(*Func))
						{
							FMDFastBindingMemberReference& MemberRef = FieldPathMembers.AddDefaulted_GetRef();
							MemberRef.bIsFunction = true;
							MemberRef.SetFromField<UFunction>(Func, false);
							NextProp = Func->GetReturnProperty();
						}
						else
						{
							FieldPathMembers.Empty();
							return;
						}
					}
				}

				if (const FObjectPropertyBase* ObjectProp = CastField<const FObjectPropertyBase>(NextProp))
				{
					OwnerStruct = ObjectProp->PropertyClass;
				}
				else if (const FStructProperty* StructProp = CastField<const FStructProperty>(NextProp))
				{
					OwnerStruct = StructProp->Struct;
				}
				else
				{
					OwnerStruct = nullptr;
				}
			}
		}

		FieldPath.Empty();
	}

	
}
