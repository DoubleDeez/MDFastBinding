#include "MDFastBindingFieldPath.h"

#include "INotifyFieldValueChanged.h"
#include "MDFastBindingHelpers.h"

FMDFastBindingFieldPath::~FMDFastBindingFieldPath()
{
	CleanupFunctionMemory();
}

bool FMDFastBindingFieldPath::BuildPath()
{
	FixupFieldPath();

	CachedPath.Empty(FieldPathMembers.Num());

	if (const UStruct* OwnerStruct = GetPathOwnerStruct())
	{
		for (const FMDFastBindingMemberReference& FieldPathMember : FieldPathMembers)
		{
			TWeakFieldPtr<const FProperty> NextProp = nullptr;
			if (FieldPathMember.bIsFunction)
			{
				UFunction* Func = FieldPathMember.ResolveMember<UFunction>();
				CachedPath.Add(Func);

				TArray<TWeakFieldPtr<const FProperty>> Params;
				FMDFastBindingHelpers::SplitFunctionParamsAndReturnProp(Func, Params, NextProp);
			}
			else if (const FProperty* Prop = FieldPathMember.ResolveMember<FProperty>())
			{
				NextProp = Prop;
				CachedPath.Add(Prop);
			}
			else if (OwnerStruct != nullptr)
			{
				// FMemberReference only supports members of UObjects, so we have to manually handle UStruct members
				const FProperty* StructProp = OwnerStruct->FindPropertyByName(FieldPathMember.GetMemberName());
				NextProp = StructProp;
				CachedPath.Add(StructProp);
			}

			if (const FObjectPropertyBase* ObjectProp = CastField<const FObjectPropertyBase>(NextProp.Get()))
			{
				OwnerStruct = ObjectProp->PropertyClass;
			}
			else if (const FStructProperty* StructProp = CastField<const FStructProperty>(NextProp.Get()))
			{
				OwnerStruct = StructProp->Struct;
			}
			else
			{
				OwnerStruct = nullptr;
			}
		}
	}

	const bool bIsPathValid = CachedPath.Num() > 0 && CachedPath.Num() == FieldPathMembers.Num();
#if WITH_EDITORONLY_DATA
	LastFrameUpdatedPath = bIsPathValid ? GFrameCounter : TOptional<uint64>();
#endif

	if (!bIsPathValid)
	{
		CachedPath.Reset();
	}

	return bIsPathValid;
}

TArray<FFieldVariant> FMDFastBindingFieldPath::GetFieldPath()
{
	const TArray<FMDFastBindingWeakFieldVariant>& WeakFieldPath = GetWeakFieldPath();

	TArray<FFieldVariant> ReturnPath;
	ReturnPath.Reserve(WeakFieldPath.Num());

	for (const FMDFastBindingWeakFieldVariant& FieldPathVariant : WeakFieldPath)
	{
		ReturnPath.Add(FieldPathVariant.GetFieldVariant());
	}

	return ReturnPath;
}

const TArray<FMDFastBindingWeakFieldVariant>& FMDFastBindingFieldPath::GetWeakFieldPath()
{
#if WITH_EDITORONLY_DATA
	// Only cached once per frame, since the user could change the path
	if (!LastFrameUpdatedPath.IsSet() || LastFrameUpdatedPath.GetValue() != GFrameCounter)
#else
	if (CachedPath.Num() == 0)
#endif
	{
		BuildPath();
	}

	return CachedPath;
}

TTuple<const FProperty*, void*> FMDFastBindingFieldPath::ResolvePath(UObject* SourceObject)
{
	void* Unused = nullptr;
	return ResolvePath(SourceObject, Unused);
}

TTuple<const FProperty*, void*> FMDFastBindingFieldPath::ResolvePath(UObject* SourceObject, void*& OutContainer)
{
	UObject* RootObject = OwnerGetter.IsBound() ? OwnerGetter.Execute(SourceObject) : nullptr;
	return ResolvePathFromRootObject(RootObject, OutContainer);
}

TTuple<const FProperty*, void*> FMDFastBindingFieldPath::ResolvePathFromRootObject(UObject* RootObject, void*& OutContainer)
{
	OutContainer = nullptr;

	void* Owner = IsValid(RootObject) ? &RootObject : nullptr;
	if (Owner != nullptr)
	{
		bool bIsOwnerAUObject = true;
		void* LastOwner = nullptr;
		const TArray<FMDFastBindingWeakFieldVariant>& Path = GetWeakFieldPath();
		for (int32 i = 0; i < Path.Num() && Owner != nullptr; ++i)
		{
			const FMDFastBindingWeakFieldVariant& FieldVariant = Path[i];
			TWeakFieldPtr<const FProperty> OwnerProp = nullptr;
			LastOwner = Owner;

			if (UFunction* Func = Cast<UFunction>(FieldVariant.ToUObject()))
			{
				UObject* OwnerUObject = *static_cast<UObject**>(LastOwner);
				if (!bIsOwnerAUObject || OwnerUObject == nullptr)
				{
					return {};
				}

				if (!OwnerUObject->IsA(Func->GetOwnerClass()))
				{
					// Func needs fixup, likely due to a reparented BP
					// TODO - Update the actual FieldVariant so this only needs to happen once
					Func = OwnerUObject->GetClass()->FindFunctionByName(Func->GetFName());
					if (Func == nullptr)
					{
						return {};
					}
				}

				void* FuncMemory = InitAndGetFunctionMemory(Func);
				if (FuncMemory == nullptr)
				{
					return {};
				}

				OwnerUObject->ProcessEvent(Func, FuncMemory);

				TArray<TWeakFieldPtr<const FProperty>> Params;
				FMDFastBindingHelpers::SplitFunctionParamsAndReturnProp(Func, Params, OwnerProp);
				Owner = FuncMemory;
			}
			else if (const FProperty* Prop = CastField<const FProperty>(FieldVariant.ToField()))
			{
				OwnerProp = Prop;

				if (bIsOwnerAUObject)
				{
					if (UObject* OwnerObject = *static_cast<UObject**>(LastOwner))
					{
						if (!OwnerObject->IsA(Prop->GetOwnerClass()))
						{
							// Prop needs fixup, likely due to a reparented BP
							// TODO - Update the actual FieldVariant so this only needs to happen once
							Prop = OwnerObject->GetClass()->FindPropertyByName(Prop->GetFName());
							if (Prop == nullptr)
							{
								return {};
							}
						}

						// Only bother allocating memory if there's a getter to use
						if (Prop->HasGetter())
						{
							Owner = InitAndGetPropertyMemory(Prop);
							if (Owner == nullptr)
							{
								return {};
							}

							Prop->GetValue_InContainer(OwnerObject, Owner);
						}
						else
						{
							Owner = Prop->ContainerPtrToValuePtr<void>(OwnerObject);
						}
					}
					else
					{
						return {};
					}
				}
				else
				{
					// Only bother allocating memory if there's a getter to use
					if (Prop->HasGetter())
					{
						Owner = InitAndGetPropertyMemory(Prop);
						if (Owner == nullptr)
						{
							return {};
						}

						Prop->GetValue_InContainer(LastOwner, Owner);
					}
					else
					{
						Owner = Prop->ContainerPtrToValuePtr<void>(LastOwner);
					}
				}
			}
			else
			{
				return {};
			}

			const bool bIsEndOfPath = i == (FieldPathMembers.Num() - 1);
			if (bIsEndOfPath)
			{
				if (bIsOwnerAUObject)
				{
					OutContainer = *static_cast<UObject**>(LastOwner);
				}
				else
				{
					OutContainer = LastOwner;
				}

				return TTuple<const FProperty*, void*>{ OwnerProp.Get(), Owner };
			}

			bIsOwnerAUObject = OwnerProp != nullptr && OwnerProp->IsA(FObjectPropertyBase::StaticClass());
		}
	}

	return { GetLeafProperty(), nullptr };
}

FFieldVariant FMDFastBindingFieldPath::GetLeafField()
{
	const TArray<FMDFastBindingWeakFieldVariant>& WeakFieldPath = GetWeakFieldPath();
	return WeakFieldPath.IsEmpty() ? FFieldVariant{} : WeakFieldPath.Last().GetFieldVariant();
}

UE::FieldNotification::FFieldId FMDFastBindingFieldPath::GetLeafFieldId()
{
	if (const FFieldVariant Field = GetLeafField())
	{
		const UClass* FieldOwner = Field.GetOwnerClass();
		if (IsValid(FieldOwner))
		{
			if (const INotifyFieldValueChanged* FieldNotify = Cast<INotifyFieldValueChanged>(FieldOwner->GetDefaultObject()))
			{
				return FieldNotify->GetFieldNotificationDescriptor().GetField(FieldOwner, Field.GetFName());
			}
		}
	}

	return {};
}

const FProperty* FMDFastBindingFieldPath::GetLeafProperty()
{
	const TArray<FMDFastBindingWeakFieldVariant>& Path = GetWeakFieldPath();

	if (Path.Num() > 0)
	{
		const FMDFastBindingWeakFieldVariant& FieldVariant = Path.Last();

		if (const UFunction* Func = Cast<UFunction>(FieldVariant.ToUObject()))
		{
			TWeakFieldPtr<const FProperty> ReturnProp = nullptr;
			TArray<TWeakFieldPtr<const FProperty>> Params;
			FMDFastBindingHelpers::SplitFunctionParamsAndReturnProp(Func, Params, ReturnProp);
			return ReturnProp.Get();
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
	const TArray<FMDFastBindingWeakFieldVariant>& Path = GetWeakFieldPath();

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
	return Prop.HasAnyPropertyFlags(CPF_BlueprintVisible)
		&& (!bOnlyAllowBlueprintReadWriteProperties || !Prop.HasAnyPropertyFlags(CPF_BlueprintReadOnly))
		&& (!FieldFilter.IsBound() || FieldFilter.Execute(&Prop));
}

bool FMDFastBindingFieldPath::IsFunctionValidForPath(const UFunction& Func) const
{
	if (!bAllowGetterFunctions || !Func.HasAnyFunctionFlags(FUNC_Const | FUNC_BlueprintPure) || (FieldFilter.IsBound() && !FieldFilter.Execute(&Func)))
	{
		return false;
	}

	TArray<TWeakFieldPtr<const FProperty>> Params;
	TWeakFieldPtr<const FProperty> ReturnProp = nullptr;
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
		const UStruct* OwnerClass = GetPathOwnerStruct();
		if (OwnerClass != nullptr && OwnerClass == VariableClass)
		{
			FieldPathMembers[0].SetMemberName(NewVariableName);
		}

		BuildPath();
	}
}
#endif

UStruct* FMDFastBindingFieldPath::GetPathOwnerStruct() const
{
	return OwnerStructGetter.IsBound() ? OwnerStructGetter.Execute() : nullptr;
}

void* FMDFastBindingFieldPath::InitAndGetFunctionMemory(const UFunction* Func)
{
	if (Func != nullptr)
	{
		if (void** MemoryPtr = FunctionMemory.Find(Func))
		{
			return *MemoryPtr;
		}

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

		return Memory;
	}

	return nullptr;
}

void FMDFastBindingFieldPath::CleanupFunctionMemory()
{
	for(const TPair<TWeakObjectPtr<const UFunction>, void*>& FuncPair : FunctionMemory)
	{
		if (FuncPair.Value != nullptr)
		{
			TArray<const FProperty*> Params;
			FMDFastBindingHelpers::GetFunctionParamProps(FuncPair.Key.Get(), Params);

			for (const FProperty* Param : Params)
			{
				Param->DestroyValue_InContainer(FuncPair.Value);
			}

			FMemory::Free(FuncPair.Value);
		}
	}

	FunctionMemory.Empty();
}

void* FMDFastBindingFieldPath::InitAndGetPropertyMemory(const FProperty* Property)
{
	if (Property != nullptr)
	{
		// const-cast to work around https://github.com/EpicGames/UnrealEngine/pull/10541
		TWeakFieldPtr<FProperty> WeakProp = MakeWeakFieldPtr(const_cast<FProperty*>(Property));
		if (void** MemoryPtr = PropertyMemory.Find(WeakProp))
		{
			return *MemoryPtr;
		}

		void* Memory = FMemory::Malloc(Property->GetSize(), Property->GetMinAlignment());
		Property->InitializeValue(Memory);
		PropertyMemory.Add(MoveTemp(WeakProp), Memory);

		return Memory;
	}

	return nullptr;
}

void FMDFastBindingFieldPath::CleanupPropertyMemory()
{
	for(const TPair<TWeakFieldPtr<FProperty>, void*>& PropertyPair : PropertyMemory)
	{
		if (PropertyPair.Value != nullptr)
		{
			if (const FProperty* Prop = PropertyPair.Key.Get())
			{
				Prop->DestroyValue(PropertyPair.Value);
			}

			FMemory::Free(PropertyPair.Value);
		}
	}

	PropertyMemory.Empty();
}

void FMDFastBindingFieldPath::FixupFieldPath()
{
	// Check if we need to convert to the Member Reference path
	if (FieldPathMembers.Num() == 0 && FieldPath.Num() != 0)
	{
		if (const UStruct* OwnerStruct = GetPathOwnerStruct())
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

	UStruct* OwnerStruct = GetPathOwnerStruct();

	// Check if PathMember needs to be updated with a new owner, likely due to a reparented, duplicated BP, or changed/renamed property type
	for (FMDFastBindingMemberReference& PathMember : FieldPathMembers)
	{
		if (OwnerStruct != nullptr)
		{
			if (UClass* OwnerClass = Cast<UClass>(OwnerStruct))
			{
				PathMember.FixUpReference(*OwnerClass);
			}

			TWeakFieldPtr<const FProperty> NextProp = nullptr;
			if (PathMember.bIsFunction)
			{
				const UFunction* Func = PathMember.ResolveMember<UFunction>();

				TArray<TWeakFieldPtr<const FProperty>> Params;
				FMDFastBindingHelpers::SplitFunctionParamsAndReturnProp(Func, Params, NextProp);
			}
			else if (const FProperty* Prop = PathMember.ResolveMember<FProperty>())
			{
				NextProp = Prop;
			}
			else if (OwnerStruct != nullptr)
			{
				NextProp = OwnerStruct->FindPropertyByName(PathMember.GetMemberName());
			}

			if (const FObjectPropertyBase* ObjectProp = CastField<const FObjectPropertyBase>(NextProp.Get()))
			{
				OwnerStruct = ObjectProp->PropertyClass;
			}
			else if (const FStructProperty* StructProp = CastField<const FStructProperty>(NextProp.Get()))
			{
				OwnerStruct = StructProp->Struct;
			}
			else
			{
				OwnerStruct = nullptr;
			}
		}
	}
}
