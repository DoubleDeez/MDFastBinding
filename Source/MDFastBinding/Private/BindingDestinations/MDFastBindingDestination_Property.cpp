#include "BindingDestinations/MDFastBindingDestination_Property.h"

#include "MDFastBinding.h"

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

bool UMDFastBindingDestination_Property::DoesBindingItemDefaultToSelf(const FName& InItemName) const
{
	return InItemName == MDFastBindingDestination_Property_Private::PathRootName;
}

void UMDFastBindingDestination_Property::InitializeDestination_Internal(UObject* SourceObject)
{
	Super::InitializeDestination_Internal(SourceObject);

	PropertyPath.BuildPath();
}

void UMDFastBindingDestination_Property::UpdateDestination_Internal(UObject* SourceObject)
{
	bool bDidUpdate = false;
	const TTuple<const FProperty*, void*> Value = GetBindingItemValue(SourceObject, MDFastBindingDestination_Property_Private::ValueSourceName, bDidUpdate);
	const TTuple<const FProperty*, void*> Property = PropertyPath.ResolvePath(SourceObject);

	if (UpdateType != EMDFastBindingUpdateType::IfUpdatesNeeded || bDidUpdate || !bHasEverUpdated || bNeedsUpdate)
	{	
		FMDFastBindingModule::SetProperty(Property.Key, Property.Value, Value.Key, Value.Value);
	}
}

void UMDFastBindingDestination_Property::PostInitProperties()
{
	PropertyPath.OwnerStructGetter.BindUObject(this, &UMDFastBindingDestination_Property::GetPropertyOwnerStruct);
	PropertyPath.OwnerGetter.BindUObject(this, &UMDFastBindingDestination_Property::GetPropertyOwner);
	
	Super::PostInitProperties();
}

UObject* UMDFastBindingDestination_Property::GetPropertyOwner(UObject* SourceObject)
{
	const TTuple<const FProperty*, void*> PathRoot = GetBindingItemValue(SourceObject, MDFastBindingDestination_Property_Private::PathRootName, bNeedsUpdate);
	if (PathRoot.Value != nullptr)
	{
		if (UObject* Owner = *static_cast<UObject**>(PathRoot.Value))
		{
			return Owner;
		}
	}

	return SourceObject;
}

UStruct* UMDFastBindingDestination_Property::GetPropertyOwnerStruct()
{
	if (const FObjectPropertyBase* ObjectProp = CastField<const FObjectPropertyBase>(GetBindingItemValueProperty(MDFastBindingDestination_Property_Private::PathRootName)))
	{
		return ObjectProp->PropertyClass;
	}
	else if (const FStructProperty* StructProp = CastField<const FStructProperty>(GetBindingItemValueProperty(MDFastBindingDestination_Property_Private::PathRootName)))
	{
		return StructProp->Struct;
	}
	
	return GetBindingOuterClass();
}

void UMDFastBindingDestination_Property::SetupBindingItems()
{
	Super::SetupBindingItems();

	EnsureBindingItemExists(MDFastBindingDestination_Property_Private::PathRootName
		, nullptr
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
	
	const FProperty* RootProp = GetBindingItemValueProperty(MDFastBindingDestination_Property_Private::PathRootName);
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

	if (PropertyPath.IsLeafFunction())
	{
		ValidationErrors.Add(LOCTEXT("LeafIsFunction", "The destination cannot be a function, please select a property"));
		Result = EDataValidationResult::Invalid;
	}

	return Result;
}

void UMDFastBindingDestination_Property::OnVariableRenamed(UClass* VariableClass, const FName& OldVariableName, const FName& NewVariableName)
{
	Super::OnVariableRenamed(VariableClass, OldVariableName, NewVariableName);

	PropertyPath.OnVariableRenamed(VariableClass, OldVariableName, NewVariableName);
}
#endif

#if WITH_EDITORONLY_DATA
FText UMDFastBindingDestination_Property::GetDisplayName()
{
	if (PropertyPath.BuildPath())
	{
		return FText::FromString(PropertyPath.ToString());
	}
	
	return Super::GetDisplayName();
}
#endif

#undef LOCTEXT_NAMESPACE
