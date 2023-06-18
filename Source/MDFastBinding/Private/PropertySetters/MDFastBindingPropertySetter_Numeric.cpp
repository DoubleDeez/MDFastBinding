#include "PropertySetters/MDFastBindingPropertySetter_Numeric.h"
#include "UObject/UnrealType.h"


void FMDFastBindingPropertySetter_Numeric::SetPropertyInContainer(const FProperty& DestinationProp, void* DestinationContainerPtr, const FProperty& SourceProp, const void* SourceValuePtr) const
{
	const FNumericProperty* DestNumericProp = CastField<const FNumericProperty>(&DestinationProp);
	const FNumericProperty* SourceNumericProp = CastField<const FNumericProperty>(&SourceProp);

	// There's no SetIntPropertyValue_InContainer or SetFloatingPointPropertyValue_InContainer so we have to manually check for each numeric type
	if (SourceNumericProp->IsInteger())
	{
		const uint64 UnsignedValue = SourceNumericProp->GetUnsignedIntPropertyValue(SourceValuePtr);
		if (DestNumericProp->IsA<FInt8Property>())
		{
			const FInt8Property::TCppType Value = UnsignedValue;
			DestNumericProp->SetValue_InContainer(DestinationContainerPtr, &Value);
		}
		else if (DestNumericProp->IsA<FInt16Property>())
		{
			const FInt16Property::TCppType Value = UnsignedValue;
			DestNumericProp->SetValue_InContainer(DestinationContainerPtr, &Value);
		}
		else if (DestNumericProp->IsA<FIntProperty>())
		{
			const FIntProperty::TCppType Value = UnsignedValue;
			DestNumericProp->SetValue_InContainer(DestinationContainerPtr, &Value);
		}
		else if (DestNumericProp->IsA<FInt64Property>())
		{
			const FInt64Property::TCppType Value = UnsignedValue;
			DestNumericProp->SetValue_InContainer(DestinationContainerPtr, &Value);
		}
		else if (DestNumericProp->IsA<FByteProperty>())
		{
			const FByteProperty::TCppType Value = UnsignedValue;
			DestNumericProp->SetValue_InContainer(DestinationContainerPtr, &Value);
		}
		else if (DestNumericProp->IsA<FUInt16Property>())
		{
			const FUInt16Property::TCppType Value = UnsignedValue;
			DestNumericProp->SetValue_InContainer(DestinationContainerPtr, &Value);
		}
		else if (DestNumericProp->IsA<FUInt32Property>())
		{
			const FUInt32Property::TCppType Value = UnsignedValue;
			DestNumericProp->SetValue_InContainer(DestinationContainerPtr, &Value);
		}
		else if (DestNumericProp->IsA<FUInt64Property>())
		{
			const FUInt64Property::TCppType Value = UnsignedValue;
			DestNumericProp->SetValue_InContainer(DestinationContainerPtr, &Value);
		}
		else if (DestNumericProp->IsA<FFloatProperty>())
		{
			const FFloatProperty::TCppType Value = UnsignedValue;
			DestNumericProp->SetValue_InContainer(DestinationContainerPtr, &Value);
		}
		else if (DestNumericProp->IsA<FDoubleProperty>())
		{
			const FDoubleProperty::TCppType Value = UnsignedValue;
			DestNumericProp->SetValue_InContainer(DestinationContainerPtr, &Value);
		}
	}
	else if (SourceNumericProp->IsFloatingPoint())
	{
		const double DoubleValue = SourceNumericProp->GetFloatingPointPropertyValue(SourceValuePtr);
		if (DestNumericProp->IsA<FInt8Property>())
		{
			const FInt8Property::TCppType Value = DoubleValue;
			DestNumericProp->SetValue_InContainer(DestinationContainerPtr, &Value);
		}
		else if (DestNumericProp->IsA<FInt16Property>())
		{
			const FInt16Property::TCppType Value = DoubleValue;
			DestNumericProp->SetValue_InContainer(DestinationContainerPtr, &Value);
		}
		else if (DestNumericProp->IsA<FIntProperty>())
		{
			const FIntProperty::TCppType Value = DoubleValue;
			DestNumericProp->SetValue_InContainer(DestinationContainerPtr, &Value);
		}
		else if (DestNumericProp->IsA<FInt64Property>())
		{
			const FInt64Property::TCppType Value = DoubleValue;
			DestNumericProp->SetValue_InContainer(DestinationContainerPtr, &Value);
		}
		else if (DestNumericProp->IsA<FByteProperty>())
		{
			const FByteProperty::TCppType Value = DoubleValue;
			DestNumericProp->SetValue_InContainer(DestinationContainerPtr, &Value);
		}
		else if (DestNumericProp->IsA<FUInt16Property>())
		{
			const FUInt16Property::TCppType Value = DoubleValue;
			DestNumericProp->SetValue_InContainer(DestinationContainerPtr, &Value);
		}
		else if (DestNumericProp->IsA<FUInt32Property>())
		{
			const FUInt32Property::TCppType Value = DoubleValue;
			DestNumericProp->SetValue_InContainer(DestinationContainerPtr, &Value);
		}
		else if (DestNumericProp->IsA<FUInt64Property>())
		{
			const FUInt64Property::TCppType Value = DoubleValue;
			DestNumericProp->SetValue_InContainer(DestinationContainerPtr, &Value);
		}
		else if (DestNumericProp->IsA<FFloatProperty>())
		{
			const FFloatProperty::TCppType Value = DoubleValue;
			DestNumericProp->SetValue_InContainer(DestinationContainerPtr, &Value);
		}
		else if (DestNumericProp->IsA<FDoubleProperty>())
		{
			const FDoubleProperty::TCppType Value = DoubleValue;
			DestNumericProp->SetValue_InContainer(DestinationContainerPtr, &Value);
		}
	}
}

void FMDFastBindingPropertySetter_Numeric::SetPropertyDirectly(const FProperty& DestinationProp, void* DestinationValuePtr, const FProperty& SourceProp, const void* SourceValuePtr) const
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
	return !DestinationProp.SameType(&SourceProp) && DestinationProp.IsA(FNumericProperty::StaticClass()) && SourceProp.IsA(FNumericProperty::StaticClass());
}

