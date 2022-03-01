// Fill out your copyright notice in the Description page of Project Settings.


#include "PropertySetters/MDFastBindingPropertySetter_Numeric.h"


void FMDFastBindingPropertySetter_Numeric::SetProperty(const FProperty& DestinationProp, void* DestinationValuePtr, const FProperty& SourceProp, const void* SourceValuePtr) const
{
	const FNumericProperty* DestNumericProp = CastField<const FNumericProperty>(&DestinationProp);
	const FNumericProperty* SourceNumericProp = CastField<const FNumericProperty>(&SourceProp);
	
	if (SourceNumericProp->IsInteger())
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
	else if (SourceNumericProp->IsFloatingPoint())
	{
		DestNumericProp->SetFloatingPointPropertyValue(DestinationValuePtr, SourceNumericProp->GetFloatingPointPropertyValue(SourceValuePtr));
	}
}

bool FMDFastBindingPropertySetter_Numeric::CanSetProperty(const FProperty& DestinationProp, const FProperty& SourceProp) const
{
	return DestinationProp.IsA(FNumericProperty::StaticClass()) && SourceProp.IsA(FNumericProperty::StaticClass());
}

