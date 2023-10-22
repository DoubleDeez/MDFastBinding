#include "PropertySetters/MDFastBindingPropertySetter_Objects.h"
#include "UObject/Field.h"
#include "UObject/UnrealType.h"

namespace MDFastBindingPropertySetter_Objects_Private
{
	template<bool bIsDestinationAContainer>
	void SetPropertyImpl(const FProperty& DestinationProp, void* DestinationPtr, const FProperty& SourceProp, const void* SourceValuePtr)
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
				if constexpr (bIsDestinationAContainer)
				{
					const FSoftObjectPtr& SoftObjectValue = SoftSrcObjProp->GetPropertyValue(SourceValuePtr);
					SoftDestObjProp->SetValue_InContainer(DestinationPtr, SoftObjectValue);
				}
				else
				{
					SoftDestObjProp->CopyCompleteValue(DestinationPtr, SourceValuePtr);
				}
			}
			else if (DestObjProp != nullptr)
			{
				if constexpr (bIsDestinationAContainer)
				{
					DestObjProp->SetObjectPropertyValue_InContainer(DestinationPtr, ObjectValue);
				}
				else
				{
					DestObjProp->SetObjectPropertyValue(DestinationPtr, ObjectValue);
				}
			}
			else if (DestInterfaceProp != nullptr)
			{
				const FScriptInterface& Interface = SrcInterfaceProp->GetPropertyValue(SourceValuePtr);
				if constexpr (bIsDestinationAContainer)
				{
					DestInterfaceProp->SetPropertyValue_InContainer(DestinationPtr, FScriptInterface(ObjectValue, Interface.GetInterface()));
				}
				else
				{
					DestInterfaceProp->SetPropertyValue(DestinationPtr, FScriptInterface(ObjectValue, Interface.GetInterface()));
				}
			}
		}
		else
		{
			if constexpr (bIsDestinationAContainer)
			{
				DestObjProp->SetObjectPropertyValue_InContainer(DestinationPtr, nullptr);
			}
			else
			{
				DestObjProp->SetObjectPropertyValue(DestinationPtr, nullptr);
			}
		}
	}
}

void FMDFastBindingPropertySetter_Objects::SetPropertyInContainer(const FProperty& DestinationProp, void* DestinationContainerPtr, const FProperty& SourceProp, const void* SourceValuePtr) const
{
	MDFastBindingPropertySetter_Objects_Private::SetPropertyImpl<true>(DestinationProp, DestinationContainerPtr, SourceProp, SourceValuePtr);
}

void FMDFastBindingPropertySetter_Objects::SetPropertyDirectly(const FProperty& DestinationProp, void* DestinationValuePtr, const FProperty& SourceProp,
	const void* SourceValuePtr) const
{
	MDFastBindingPropertySetter_Objects_Private::SetPropertyImpl<false>(DestinationProp, DestinationValuePtr, SourceProp, SourceValuePtr);
}

bool FMDFastBindingPropertySetter_Objects::CanSetProperty(const FProperty& DestinationProp, const FProperty& SourceProp) const
{
	// Ignore same types
	if (DestinationProp.SameType(&SourceProp))
	{
		return false;
	}

	const FObjectPropertyBase* DestObjProp = CastField<const FObjectPropertyBase>(&DestinationProp);
	const FObjectPropertyBase* SrcObjProp = CastField<const FObjectPropertyBase>(&SourceProp);
	const FInterfaceProperty* DestInterfaceProp = CastField<const FInterfaceProperty>(&DestinationProp);
	const FInterfaceProperty* SrcInterfaceProp = CastField<const FInterfaceProperty>(&SourceProp);

	if (SrcObjProp != nullptr && DestObjProp != nullptr)
	{
		if (!SrcObjProp->PropertyClass->IsChildOf(DestObjProp->PropertyClass) && !DestObjProp->PropertyClass->IsChildOf(SrcObjProp->PropertyClass))
		{
			return false;
		}
	}

	return (DestObjProp != nullptr || DestInterfaceProp != nullptr) && (SrcObjProp != nullptr || SrcInterfaceProp != nullptr);
}
