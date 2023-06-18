#include "PropertySetters/MDFastBindingPropertySetter_Colors.h"
#include "Styling/SlateColor.h"
#include "UObject/Class.h"
#include "UObject/Field.h"
#include "UObject/UnrealType.h"

namespace MDFastBindingPropertySetter_Colors_Private
{
	template<bool bIsDestinationAContainer>
	void SetPropertyImpl(const FProperty& DestinationProp, void* DestinationPtr, const FProperty& SourceProp, const void* SourceValuePtr)
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
				if constexpr (bIsDestinationAContainer)
				{
					DestStructProp->SetValue_InContainer(DestinationPtr, &Value);
				}
				else
				{
					DestStructProp->CopyCompleteValue(DestinationPtr, &Value);
				}
			}
			else if (SourceStructProp->Struct == TBaseStructure<FSlateColor>::Get())
			{
				const FLinearColor Value = static_cast<const FSlateColor*>(SourceValuePtr)->GetSpecifiedColor();
				if constexpr (bIsDestinationAContainer)
				{
					DestStructProp->SetValue_InContainer(DestinationPtr, &Value);
				}
				else
				{
					DestStructProp->CopyCompleteValue(DestinationPtr, &Value);
				}
			}
		}
		else if (DestStructProp->Struct == TBaseStructure<FColor>::Get())
		{
			if (SourceStructProp->Struct == TBaseStructure<FLinearColor>::Get())
			{
				// Explicit conversion should be done for SRGB
				constexpr bool bSRGB = false;
				const FColor Value = static_cast<const FLinearColor*>(SourceValuePtr)->ToFColor(bSRGB);
				if constexpr (bIsDestinationAContainer)
				{
					DestStructProp->SetValue_InContainer(DestinationPtr, &Value);
				}
				else
				{
					DestStructProp->CopyCompleteValue(DestinationPtr, &Value);
				}

			}
			else if (SourceStructProp->Struct == TBaseStructure<FSlateColor>::Get())
			{
				// Explicit conversion should be done for SRGB
				constexpr bool bSRGB = false;
				const FColor Value = static_cast<const FSlateColor*>(SourceValuePtr)->GetSpecifiedColor().ToFColor(bSRGB);
				if constexpr (bIsDestinationAContainer)
				{
					DestStructProp->SetValue_InContainer(DestinationPtr, &Value);
				}
				else
				{
					DestStructProp->CopyCompleteValue(DestinationPtr, &Value);
				}
			}
		}
		else if (DestStructProp->Struct == TBaseStructure<FSlateColor>::Get())
		{
			if (SourceStructProp->Struct == TBaseStructure<FLinearColor>::Get())
			{
				const FSlateColor Value = *static_cast<const FLinearColor*>(SourceValuePtr);
				if constexpr (bIsDestinationAContainer)
				{
					DestStructProp->SetValue_InContainer(DestinationPtr, &Value);
				}
				else
				{
					DestStructProp->CopyCompleteValue(DestinationPtr, &Value);
				}
			}
			else if (SourceStructProp->Struct == TBaseStructure<FColor>::Get())
			{
				const FSlateColor Value = *static_cast<const FColor*>(SourceValuePtr);
				if constexpr (bIsDestinationAContainer)
				{
					DestStructProp->SetValue_InContainer(DestinationPtr, &Value);
				}
				else
				{
					DestStructProp->CopyCompleteValue(DestinationPtr, &Value);
				}
			}
		}
	}
}

void FMDFastBindingPropertySetter_Colors::SetPropertyInContainer(const FProperty& DestinationProp, void* DestinationContainerPtr, const FProperty& SourceProp, const void* SourceValuePtr) const
{
	MDFastBindingPropertySetter_Colors_Private::SetPropertyImpl<true>(DestinationProp, DestinationContainerPtr, SourceProp, SourceValuePtr);
}

void FMDFastBindingPropertySetter_Colors::SetPropertyDirectly(const FProperty& DestinationProp, void* DestinationValuePtr, const FProperty& SourceProp, const void* SourceValuePtr) const
{
	MDFastBindingPropertySetter_Colors_Private::SetPropertyImpl<false>(DestinationProp, DestinationValuePtr, SourceProp, SourceValuePtr);
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
