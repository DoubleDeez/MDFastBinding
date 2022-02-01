#include "PropertySetters/MDFastBindingPropertySetter_Objects.h"


FMDFastBindingPropertySetter_Objects::FMDFastBindingPropertySetter_Objects()
	: SupportedFieldTypes({ FObjectProperty::StaticClass(), FSoftObjectProperty::StaticClass()
		, FWeakObjectProperty::StaticClass(), FLazyObjectProperty::StaticClass() })
{
}

void FMDFastBindingPropertySetter_Objects::SetProperty(const FProperty& DestinationProp, void* DestinationValuePtr, const FProperty& SourceProp, const void* SourceValuePtr) const
{
	const FObjectPropertyBase* DestObjProp = CastField<const FObjectPropertyBase>(&DestinationProp);
	const FObjectPropertyBase* SrcObjProp = CastField<const FObjectPropertyBase>(&SourceProp);

	UObject* ObjectValue = SrcObjProp->GetObjectPropertyValue(SourceValuePtr);
	DestObjProp->SetObjectPropertyValue(DestinationValuePtr, ObjectValue);
}

bool FMDFastBindingPropertySetter_Objects::CanSetProperty(const FProperty& DestinationProp, const FProperty& SourceProp) const
{
	if (!IMDFastBindingPropertySetter::CanSetProperty(DestinationProp, SourceProp))
	{
		return false;
	}

	const FObjectPropertyBase* DestObjProp = CastField<const FObjectPropertyBase>(&DestinationProp);
	const FObjectPropertyBase* SrcObjProp = CastField<const FObjectPropertyBase>(&SourceProp);
	if (DestObjProp == nullptr || SrcObjProp == nullptr)
	{
		return false;
	}

	return SrcObjProp->PropertyClass->IsChildOf(DestObjProp->PropertyClass);
}
