#include "BindingDestinations/MDFastBindingDestination_Property.h"

#define LOCTEXT_NAMESPACE "MDFastBindingDestination_Property"

namespace MDFastBindingDestination_Property_Private
{
	const FName PathRootName = TEXT("Path Root");
	const FName ValueSourceName = TEXT("Value Source");
}

UMDFastBindingDestination_Property::UMDFastBindingDestination_Property()
{
	PropertyPath.bOnlyAllowBlueprintReadWriteProperties = true;
}

void UMDFastBindingDestination_Property::InitializeDestination_Internal(UObject* SourceObject)
{
	Super::InitializeDestination_Internal(SourceObject);

	PropertyPath.BuildPath();
}

void UMDFastBindingDestination_Property::UpdateDestination_Internal(UObject* SourceObject)
{
	const TTuple<const FProperty*, void*> Value = GetBindingItemValue(SourceObject, MDFastBindingDestination_Property_Private::ValueSourceName);
	if (Value.Key ==  nullptr || Value.Value == nullptr)
	{
		return;
	}

	const TTuple<const FProperty*, void*> Property = PropertyPath.ResolvePath(SourceObject);
	if (Property.Key == nullptr || Property.Value == nullptr)
	{
		return;
	}
	
	// TODO - Converters
	if (!Property.Key->SameType(Value.Key))
	{
		return;
	}
	
	Property.Key->CopyCompleteValue(Property.Value, Value.Value);
}

void UMDFastBindingDestination_Property::PostInitProperties()
{
	Super::PostInitProperties();
	
	PropertyPath.OwnerClassGetter.BindUObject(this, &UMDFastBindingDestination_Property::GetPropertyOwnerClass);
	PropertyPath.OwnerGetter.BindUObject(this, &UMDFastBindingDestination_Property::GetPropertyOwner);
}

UObject* UMDFastBindingDestination_Property::GetPropertyOwner(UObject* SourceObject)
{
	const TTuple<const FProperty*, void*> PathRoot = GetBindingItemValue(SourceObject, MDFastBindingDestination_Property_Private::PathRootName);
	if (PathRoot.Value != nullptr)
	{
		return *static_cast<UObject**>(PathRoot.Value);
	}

	return SourceObject;
}

UClass* UMDFastBindingDestination_Property::GetPropertyOwnerClass()
{
	if (const FObjectPropertyBase* ObjectProp = CastField<const FObjectPropertyBase>(GetBindingItemValueProperty(MDFastBindingDestination_Property_Private::PathRootName)))
	{
		return ObjectProp->PropertyClass;
	}
	
	return GetBindingOuterClass();
}

void UMDFastBindingDestination_Property::SetupBindingItems()
{
	Super::SetupBindingItems();

	EnsureBindingItemExists(MDFastBindingDestination_Property_Private::PathRootName
		, GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UMDFastBindingDestination_Property, ObjectProperty))
		, LOCTEXT("PathRootToolTip", "The root object that has the property to set the value of. (Defaults to 'Self').")
		, true);
	EnsureBindingItemExists(MDFastBindingDestination_Property_Private::ValueSourceName
		, PropertyPath.GetLeafProperty()
		, LOCTEXT("ValueSourceToolTip", "The value to assign to the specified property"));
}

#if WITH_EDITOR
EDataValidationResult UMDFastBindingDestination_Property::IsDataValid(TArray<FText>& ValidationErrors)
{
	EDataValidationResult Result = Super::IsDataValid(ValidationErrors);
	
	if (!PropertyPath.BuildPath())
	{
		ValidationErrors.Add(LOCTEXT("FailedToBuildPath", "Could not build path to property, please select a valid property path"));
		Result = EDataValidationResult::Invalid;
	}

	if (PropertyPath.IsLeafFunction())
	{
		ValidationErrors.Add(LOCTEXT("LeafIsFunction", "The destination cannot be a function, please select a property"));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
