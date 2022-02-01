#include "BindingValues/MDFastBindingValue_Property.h"

#define LOCTEXT_NAMESPACE "MDFastBindingDestination_Property"

namespace MDFastBindingValue_Property_Private
{
	const FName PathRootName = TEXT("Path Root");
}

TTuple<const FProperty*, void*> UMDFastBindingValue_Property::GetValue(UObject* SourceObject)
{
	return PropertyPath.ResolvePath(SourceObject);
}

const FProperty* UMDFastBindingValue_Property::GetOutputProperty()
{
	return PropertyPath.GetLeafProperty();
}

UObject* UMDFastBindingValue_Property::GetPropertyOwner(UObject* SourceObject)
{
	const TTuple<const FProperty*, void*> PathRoot = GetBindingItemValue(SourceObject, MDFastBindingValue_Property_Private::PathRootName);
	if (PathRoot.Value != nullptr)
	{
		return *static_cast<UObject**>(PathRoot.Value);
	}
	
	return SourceObject;
}

UClass* UMDFastBindingValue_Property::GetPropertyOwnerClass()
{
	if (const FObjectPropertyBase* ObjectProp = CastField<const FObjectPropertyBase>(GetBindingItemValueProperty(MDFastBindingValue_Property_Private::PathRootName)))
	{
		return ObjectProp->PropertyClass;
	}
	
	return GetBindingOuterClass();
}

void UMDFastBindingValue_Property::SetupBindingItems()
{
	Super::SetupBindingItems();

	EnsureBindingItemExists(MDFastBindingValue_Property_Private::PathRootName
		, GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UMDFastBindingValue_Property, ObjectProperty))
		, LOCTEXT("PathRootToolTip", "The root object that has the property to get the value of. (Defaults to 'Self').")
		, true);
}

void UMDFastBindingValue_Property::PostInitProperties()
{
	Super::PostInitProperties();
	
	PropertyPath.OwnerClassGetter.BindUObject(this, &UMDFastBindingValue_Property::GetPropertyOwnerClass);
	PropertyPath.OwnerGetter.BindUObject(this, &UMDFastBindingValue_Property::GetPropertyOwner);
}

#if WITH_EDITOR
EDataValidationResult UMDFastBindingValue_Property::IsDataValid(TArray<FText>& ValidationErrors)
{
	EDataValidationResult Result = Super::IsDataValid(ValidationErrors);
	
	if (!PropertyPath.BuildPath())
	{
		ValidationErrors.Add(LOCTEXT("FailedToBuildPath", "Could not build path to property, please select a valid property path"));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
