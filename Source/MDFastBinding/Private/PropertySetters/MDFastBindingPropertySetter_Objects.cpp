#include "PropertySetters/MDFastBindingPropertySetter_Objects.h"
#include "UObject/Field.h"
#include "UObject/UnrealType.h"

void FMDFastBindingPropertySetter_Objects::SetProperty(const FProperty& DestinationProp, void* DestinationValuePtr, const FProperty& SourceProp, const void* SourceValuePtr) const
{
	const FObjectPropertyBase* DestObjProp = CastField<const FObjectPropertyBase>(&DestinationProp);
	const FObjectPropertyBase* SrcObjProp = CastField<const FObjectPropertyBase>(&SourceProp);
	const FInterfaceProperty* DestInterfaceProp = CastField<const FInterfaceProperty>(&DestinationProp);
	const FInterfaceProperty* SrcInterfaceProp = CastField<const FInterfaceProperty>(&SourceProp);

	UObject* ObjectValue = (SrcObjProp != nullptr) ? SrcObjProp->GetObjectPropertyValue(SourceValuePtr) : SrcInterfaceProp->GetPropertyValue(SourceValuePtr).GetObject();
	const UClass* SourcePropClass = (SrcObjProp != nullptr) ? SrcObjProp->PropertyClass : SrcInterfaceProp->InterfaceClass;
	const UClass* DestinationClass = (DestObjProp != nullptr) ? DestObjProp->PropertyClass : DestInterfaceProp->InterfaceClass;

	// Check if the declared types are compatible, if not, check if the actual type is compatible
	const bool bCanAssign = (SourcePropClass->IsChildOf(DestinationClass))
		|| (ObjectValue != nullptr && ObjectValue->IsA(DestinationClass));
	
	if (bCanAssign)
	{
		// Special case for soft object ptrs since we want them to stay soft object ptrs
		const FSoftObjectProperty* SoftDestObjProp = CastField<const FSoftObjectProperty>(&DestinationProp);
		const FSoftObjectProperty* SoftSrcObjProp = CastField<const FSoftObjectProperty>(&SourceProp);
		if (SoftDestObjProp != nullptr && SoftSrcObjProp != nullptr)
		{
			SoftDestObjProp->CopyCompleteValue(DestinationValuePtr, SourceValuePtr);
		}
		else if (DestObjProp != nullptr)
		{
			DestObjProp->SetObjectPropertyValue(DestinationValuePtr, ObjectValue);
		}
		else if (DestInterfaceProp != nullptr)
		{
			const FScriptInterface& Interface = SrcInterfaceProp->GetPropertyValue(SourceValuePtr);
			DestInterfaceProp->SetPropertyValue(DestinationValuePtr, FScriptInterface(ObjectValue, Interface.GetInterface()));
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
	const FInterfaceProperty* DestInterfaceProp = CastField<const FInterfaceProperty>(&DestinationProp);
	const FInterfaceProperty* SrcInterfaceProp = CastField<const FInterfaceProperty>(&SourceProp);
	return (DestObjProp != nullptr || DestInterfaceProp != nullptr) && (SrcObjProp != nullptr || SrcInterfaceProp != nullptr);
}
