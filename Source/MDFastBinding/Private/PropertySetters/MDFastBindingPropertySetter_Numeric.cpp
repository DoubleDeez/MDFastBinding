#include "PropertySetters/MDFastBindingPropertySetter_Numeric.h"
#include "UObject/UnrealType.h"


void FMDFastBindingPropertySetter_Numeric::SetProperty(const FProperty& DestinationProp, void* DestinationValuePtr, const FProperty& SourceProp, const void* SourceValuePtr) const
{
	const FNumericProperty* DestNumericProp = CastField<const FNumericProperty>(&DestinationProp);
	const FNumericProperty* SourceNumericProp = CastField<const FNumericProperty>(&SourceProp);
	
	if (SourceNumericProp->IsInteger())
	{
		if (DestNumericProp->IsInteger())
		{
			const uint64 UnsignedValue = SourceNumericProp->GetUnsignedIntPropertyValue(SourceValuePtr);
			if (DestNumericProp->CanHoldValue(UnsignedValue))
			{
				DestNumericProp->SetIntPropertyValue(DestinationValuePtr, UnsignedValue);
			}
			else
			{
				DestNumericProp->SetIntPropertyValue(DestinationValuePtr, SourceNumericProp->GetSignedIntPropertyValue(SourceValuePtr));
			}
		}
		else if (DestNumericProp->IsFloatingPoint())
		{
			const uint64 UnsignedValue = SourceNumericProp->GetUnsignedIntPropertyValue(SourceValuePtr);
			if (DestNumericProp->CanHoldValue(UnsignedValue))
			{
				DestNumericProp->SetFloatingPointPropertyValue(DestinationValuePtr, UnsignedValue);
			}
			else
			{
				DestNumericProp->SetFloatingPointPropertyValue(DestinationValuePtr, SourceNumericProp->GetSignedIntPropertyValue(SourceValuePtr));
			}
		}
	}
	else if (SourceNumericProp->IsFloatingPoint())
	{
		if (DestNumericProp->IsInteger())
		{
			const uint64 UnsignedValue = SourceNumericProp->GetFloatingPointPropertyValue(SourceValuePtr);
			if (DestNumericProp->CanHoldValue(UnsignedValue))
			{
				DestNumericProp->SetFloatingPointPropertyValue(DestinationValuePtr, UnsignedValue);
			}
			else
			{
				const int32 SignedValue = SourceNumericProp->GetFloatingPointPropertyValue(SourceValuePtr);
				DestNumericProp->SetFloatingPointPropertyValue(DestinationValuePtr, SignedValue);
			}
		}
		else if (DestNumericProp->IsFloatingPoint())
		{
			DestNumericProp->SetFloatingPointPropertyValue(DestinationValuePtr, SourceNumericProp->GetFloatingPointPropertyValue(SourceValuePtr));
		}
	}
}

bool FMDFastBindingPropertySetter_Numeric::CanSetProperty(const FProperty& DestinationProp, const FProperty& SourceProp) const
{
	return DestinationProp.IsA(FNumericProperty::StaticClass()) && SourceProp.IsA(FNumericProperty::StaticClass());
}

