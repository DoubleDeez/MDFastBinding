#include "PropertySetters/MDFastBindingPropertySetter_Objects.h"
#include "UObject/Field.h"
#include "UObject/UnrealType.h"

void FMDFastBindingPropertySetter_Objects::SetProperty(const FProperty& DestinationProp, void* DestinationValuePtr, const FProperty& SourceProp, const void* SourceValuePtr) const
{
	const FObjectPropertyBase* DestObjProp = CastField<const FObjectPropertyBase>(&DestinationProp);
	const FObjectPropertyBase* SrcObjProp = CastField<const FObjectPropertyBase>(&SourceProp);

	UObject* ObjectValue = SrcObjProp->GetObjectPropertyValue(SourceValuePtr);

	// Check if the declared types are compatible, if not, check if the actual type is compatible
	const bool bCanAssign = (SrcObjProp->PropertyClass->IsChildOf(DestObjProp->PropertyClass))
		|| (ObjectValue != nullptr && ObjectValue->IsA(DestObjProp->PropertyClass));
	
	if (bCanAssign)
	{
		// Special case for soft object ptrs since we want them to stay soft object ptrs
		const FSoftObjectProperty* SoftDestObjProp = CastField<const FSoftObjectProperty>(&DestinationProp);
		const FSoftObjectProperty* SoftSrcObjProp = CastField<const FSoftObjectProperty>(&SourceProp);
		if (SoftDestObjProp != nullptr && SoftSrcObjProp != nullptr)
		{
			SoftDestObjProp->CopyCompleteValue(DestinationValuePtr, SourceValuePtr);
		}
		else
		{
			DestObjProp->SetObjectPropertyValue(DestinationValuePtr, ObjectValue);
		}
	}
	else
	{
		DestObjProp->SetObjectPropertyValue(DestinationValuePtr, nullptr);
	}
}

bool FMDFastBindingPropertySetter_Objects::CanSetProperty(const FProperty& DestinationProp, const FProperty& SourceProp) const
{
	const FObjectPropertyBase* DestObjProp = CastField<const FObjectPropertyBase>(&DestinationProp);
	const FObjectPropertyBase* SrcObjProp = CastField<const FObjectPropertyBase>(&SourceProp);
	return DestObjProp != nullptr && SrcObjProp != nullptr;
}
