#include "BindingValues/MDFastBindingValue_Property.h"

#define LOCTEXT_NAMESPACE "MDFastBindingValue_Property"

namespace MDFastBindingValue_Property_Private
{
	const FName PathRootName = TEXT("Path Root");
}

UMDFastBindingValue_Property::UMDFastBindingValue_Property()
{
	// Make the default match behaviour since the property value could have changed, we ignore EMDFastBindingUpdateType::IfUpdatesNeeded
	UpdateType = EMDFastBindingUpdateType::Always;
}

TTuple<const FProperty*, void*> UMDFastBindingValue_Property::GetValue_Internal(UObject* SourceObject)
{
	return PropertyPath.ResolvePath(SourceObject);
}

const FProperty* UMDFastBindingValue_Property::GetOutputProperty()
{
	return PropertyPath.GetLeafProperty();
}

bool UMDFastBindingValue_Property::DoesBindingItemDefaultToSelf(const FName& InItemName) const
{
	return InItemName == MDFastBindingValue_Property_Private::PathRootName;
}

#if WITH_EDITORONLY_DATA
FText UMDFastBindingValue_Property::GetDisplayName()
{
	if (PropertyPath.BuildPath())
	{
		return FText::FromString(PropertyPath.ToString());
	}

	return Super::GetDisplayName();
}
#endif

UObject* UMDFastBindingValue_Property::GetPropertyOwner(UObject* SourceObject)
{
	bool bDidUpdate = false;
	const TTuple<const FProperty*, void*> PathRoot = GetBindingItemValue(SourceObject, MDFastBindingValue_Property_Private::PathRootName, bDidUpdate);

	if (PathRoot.Value != nullptr)
	{
		return *static_cast<UObject**>(PathRoot.Value);
	}
	else if (PathRoot.Key != nullptr)
	{
		// null value, but key is valid so it failed to get a value, just return null as the owner
		return nullptr;
	}

	return SourceObject;
}

UStruct* UMDFastBindingValue_Property::GetPropertyOwnerStruct()
{
	const FProperty* RootProp = GetBindingItemValueProperty(MDFastBindingValue_Property_Private::PathRootName);
	if (const FObjectPropertyBase* ObjectProp = CastField<const FObjectPropertyBase>(RootProp))
	{
		return ObjectProp->PropertyClass;
	}
	else if (const FStructProperty* StructProp = CastField<const FStructProperty>(RootProp))
	{
		return StructProp->Struct;
	}

	return GetBindingOwnerClass();
}

void UMDFastBindingValue_Property::SetupBindingItems()
{
	Super::SetupBindingItems();

	EnsureBindingItemExists(MDFastBindingValue_Property_Private::PathRootName
		, GetPathRootProperty()
		, LOCTEXT("PathRootToolTip", "The root object that has the property to get the value of. (Defaults to 'Self').")
		, true);
}

void UMDFastBindingValue_Property::PostInitProperties()
{
	PropertyPath.OwnerStructGetter.BindUObject(this, &UMDFastBindingValue_Property::GetPropertyOwnerStruct);
	PropertyPath.OwnerGetter.BindUObject(this, &UMDFastBindingValue_Property::GetPropertyOwner);

	Super::PostInitProperties();
}

#if WITH_EDITOR
EDataValidationResult UMDFastBindingValue_Property::IsDataValid(TArray<FText>& ValidationErrors)
{
	EDataValidationResult Result = Super::IsDataValid(ValidationErrors);

	const FProperty* RootProp = GetBindingItemValueProperty(MDFastBindingValue_Property_Private::PathRootName);
	if (RootProp != nullptr && !RootProp->IsA<FObjectPropertyBase>() && !RootProp->IsA<FStructProperty>())
	{
		ValidationErrors.Add(LOCTEXT("InvalidRootProperty", "Path root must be a UObject or struct type property"));
		Result = EDataValidationResult::Invalid;
	}

	if (!PropertyPath.BuildPath())
	{
		ValidationErrors.Add(LOCTEXT("FailedToBuildPath", "Could not build path to property, please select a valid property path"));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}

void UMDFastBindingValue_Property::OnVariableRenamed(UClass* VariableClass, const FName& OldVariableName, const FName& NewVariableName)
{
	Super::OnVariableRenamed(VariableClass, OldVariableName, NewVariableName);

	PropertyPath.OnVariableRenamed(VariableClass, OldVariableName, NewVariableName);
}

void UMDFastBindingValue_Property::SetFieldPath(const TArray<FFieldVariant>& Path)
{
	PropertyPath.FieldPathMembers.Empty();
	for (const FFieldVariant& Field : Path)
	{
		FMDFastBindingMemberReference& MemberRef = PropertyPath.FieldPathMembers.AddDefaulted_GetRef();
		if (Field.IsA<UFunction>())
		{
			MemberRef.SetFromField<UFunction>(Field.Get<UFunction>(), false);
			MemberRef.bIsFunction = true;
		}
		else
		{
			MemberRef.SetFromField<FProperty>(Field.Get<FProperty>(), false);
			MemberRef.bIsFunction = false;
		}
	}

	PropertyPath.BuildPath();
}

const TArray<FFieldVariant>& UMDFastBindingValue_Property::GetFieldPath()
{
	return PropertyPath.GetFieldPath();
}
#endif

#undef LOCTEXT_NAMESPACE
