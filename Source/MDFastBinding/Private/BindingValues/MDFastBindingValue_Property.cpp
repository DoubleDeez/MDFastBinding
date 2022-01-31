#include "BindingValues/MDFastBindingValue_Property.h"

#define LOCTEXT_NAMESPACE "MDFastBindingDestination_Property"

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
	return SourceObject;
}

UClass* UMDFastBindingValue_Property::GetPropertyOwnerClass()
{
	return GetBindingOuterClass();
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
