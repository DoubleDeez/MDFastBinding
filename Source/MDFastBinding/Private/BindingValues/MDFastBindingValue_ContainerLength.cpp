#include "BindingValues/MDFastBindingValue_ContainerLength.h"

#define LOCTEXT_NAMESPACE "MDFastBindingValue_ContainerLength"

namespace MDFastBindingValue_ContainerLength_Private
{
	const FName ContainerName = TEXT("Container");
}

const FProperty* UMDFastBindingValue_ContainerLength::GetOutputProperty()
{
	if (Int32Prop == nullptr)
	{
		Int32Prop = GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UMDFastBindingValue_ContainerLength, OutputValue));
	}

	return Int32Prop;
}

#if WITH_EDITORONLY_DATA
FText UMDFastBindingValue_ContainerLength::GetDisplayName()
{
	if (const FProperty* ContainerProp = GetBindingItemValueProperty(MDFastBindingValue_ContainerLength_Private::ContainerName))
	{
		static const FText Format = LOCTEXT("DisplayNameFormat", "Length of {0}");
		return FText::Format(Format, ContainerProp->GetDisplayNameText());
	}
	return Super::GetDisplayName();
}
#endif

#if WITH_EDITOR
EDataValidationResult UMDFastBindingValue_ContainerLength::IsDataValid(TArray<FText>& ValidationErrors)
{
	if (const FProperty* ContainerProp = GetBindingItemValueProperty(MDFastBindingValue_ContainerLength_Private::ContainerName))
	{
		const bool bIsValidType = ContainerProp->IsA<FArrayProperty>() || ContainerProp->IsA<FSetProperty>() || ContainerProp->IsA<FMapProperty>();
		if (!bIsValidType)
		{
			ValidationErrors.Add(LOCTEXT("IncorrectTypeError", "Container Pin is not connected to a container type. (Array, Set, or Map)"));
			return EDataValidationResult::Invalid;
		}
	}

	return Super::IsDataValid(ValidationErrors);
}
#endif

TTuple<const FProperty*, void*> UMDFastBindingValue_ContainerLength::GetValue_Internal(UObject* SourceObject)
{
	bool bDidUpdate = false;
	const TTuple<const FProperty*, void*> Container = GetBindingItemValue(SourceObject, MDFastBindingValue_ContainerLength_Private::ContainerName, bDidUpdate);
	if (Container.Key != nullptr && Container.Value != nullptr && bDidUpdate)
	{
		OutputValue = 0;
		
		if (const FArrayProperty* ArrayProp = CastField<const FArrayProperty>(Container.Key))
		{
			const FScriptArrayHelper Helper = FScriptArrayHelper(ArrayProp, Container.Value);
			OutputValue = Helper.Num();
		}
		else  if (const FSetProperty* SetProp = CastField<const FSetProperty>(Container.Key))
		{
			const FScriptSetHelper Helper = FScriptSetHelper(SetProp, Container.Value);
			OutputValue = Helper.Num();
		}
		else if (const FMapProperty* MapProp = CastField<const FMapProperty>(Container.Key))
		{
			const FScriptMapHelper Helper = FScriptMapHelper(MapProp, Container.Value);
			OutputValue = Helper.Num();
		}
	}

	return TTuple<const FProperty*, void*>{ GetOutputProperty(), &OutputValue };
}

void UMDFastBindingValue_ContainerLength::SetupBindingItems()
{
	EnsureBindingItemExists(MDFastBindingValue_ContainerLength_Private::ContainerName, nullptr
		, LOCTEXT("ContainerPinDescription", "The container (Array, Set, Map) to get the length of."));
}

#undef LOCTEXT_NAMESPACE
