#include "PropertySetters/MDFastBindingPropertySetter_Colors.h"
#include "Styling/SlateColor.h"
#include "UObject/Class.h"
#include "UObject/Field.h"
#include "UObject/UnrealType.h"


void FMDFastBindingPropertySetter_Colors::SetProperty(const FProperty& DestinationProp, void* DestinationValuePtr, const FProperty& SourceProp, const void* SourceValuePtr) const
{
	const FStructProperty* DestStructProp = CastField<const FStructProperty>(&DestinationProp);
	const FStructProperty* SourceStructProp = CastField<const FStructProperty>(&SourceProp);
	if (DestStructProp == nullptr && SourceStructProp == nullptr)
	{
		return;
	}

	if (DestStructProp->Struct == TBaseStructure<FLinearColor>::Get())
	{
		if (SourceStructProp->Struct == TBaseStructure<FColor>::Get())
		{
			const FLinearColor Value = *static_cast<const FColor*>(SourceValuePtr);
			DestStructProp->CopyCompleteValue(DestinationValuePtr, &Value);
		}
		else if (SourceStructProp->Struct == TBaseStructure<FSlateColor>::Get())
		{
			const FLinearColor Value = static_cast<const FSlateColor*>(SourceValuePtr)->GetSpecifiedColor();
			DestStructProp->CopyCompleteValue(DestinationValuePtr, &Value);
		}
	}
	else if (DestStructProp->Struct == TBaseStructure<FColor>::Get())
	{
		if (SourceStructProp->Struct == TBaseStructure<FLinearColor>::Get())
		{
			// Explicit conversion should be done for SRGB
			constexpr bool bSRGB = false;
			const FColor Value = static_cast<const FLinearColor*>(SourceValuePtr)->ToFColor(bSRGB);
			DestStructProp->CopyCompleteValue(DestinationValuePtr, &Value);
		
		}
		else if (SourceStructProp->Struct == TBaseStructure<FSlateColor>::Get())
		{
			// Explicit conversion should be done for SRGB
			constexpr bool bSRGB = false;
			const FColor Value = static_cast<const FSlateColor*>(SourceValuePtr)->GetSpecifiedColor().ToFColor(bSRGB);
			DestStructProp->CopyCompleteValue(DestinationValuePtr, &Value);
		}
	}
	else if (DestStructProp->Struct == TBaseStructure<FSlateColor>::Get())
	{
		if (SourceStructProp->Struct == TBaseStructure<FLinearColor>::Get())
		{
			const FSlateColor Value = *static_cast<const FLinearColor*>(SourceValuePtr);
			DestStructProp->CopyCompleteValue(DestinationValuePtr, &Value);
		}
		else if (SourceStructProp->Struct == TBaseStructure<FColor>::Get())
		{
			const FSlateColor Value = *static_cast<const FColor*>(SourceValuePtr);
			DestStructProp->CopyCompleteValue(DestinationValuePtr, &Value);
		}
	}
}

bool FMDFastBindingPropertySetter_Colors::CanSetProperty(const FProperty& DestinationProp, const FProperty& SourceProp) const
{
	static TSet<UScriptStruct*> ColorStructs = { TBaseStructure<FColor>::Get(), TBaseStructure<FLinearColor>::Get(), TBaseStructure<FSlateColor>::Get() };

	// Ignore same types
	if (DestinationProp.SameType(&SourceProp))
	{
		return false;
	}

	const FStructProperty* DestStructProp = CastField<const FStructProperty>(&DestinationProp);
	const FStructProperty* SourceStructProp = CastField<const FStructProperty>(&SourceProp);
	return DestStructProp != nullptr && SourceStructProp != nullptr
			&& ColorStructs.Contains(DestStructProp->Struct) && ColorStructs.Contains(SourceStructProp->Struct);
}
