#include "MDFastBindingObject.h"
#include "MDFastBindingContainer.h"
#include "BindingValues/MDFastBindingValueBase.h"

#define LOCTEXT_NAMESPACE "MDFastBindingObject"

UClass* UMDFastBindingObject::GetBindingOuterClass() const
{
#if !WITH_EDITOR
	if (BindingOuterClass.IsValid())
	{
		return BindingOuterClass.Get();
	}
#endif

	const UObject* Object = this;
	while (Object != nullptr)
	{
		if (Object->IsA<UMDFastBindingContainer>() && Object->GetOuter() != nullptr)
		{
			UClass* Class = Object->GetOuter()->GetClass();
			BindingOuterClass = Class;
			return Class;
		}

		Object = Object->GetOuter();
	}

	return nullptr;
}

void UMDFastBindingObject::PostLoad()
{
	Super::PostLoad();

	SetupBindingItems();
}

void UMDFastBindingObject::PostInitProperties()
{
	Super::PostInitProperties();

	SetupBindingItems();
}

void UMDFastBindingObject::EnsureBindingItemExists(const FName& ItemName, const FProperty* ItemProperty, const FText& ItemDescription, bool bIsOptional)
{
	FMDFastBindingItem* BindingItem = BindingItems.FindByKey(ItemName);
	if (BindingItem == nullptr)
	{
		BindingItems.Add({ ItemName });
		BindingItem = BindingItems.FindByKey(ItemName);
	}

	BindingItem->bAllowNullValue = bIsOptional;
	BindingItem->ItemProperty = ItemProperty;
	BindingItem->ToolTip = ItemDescription;
}

const FProperty* UMDFastBindingObject::GetBindingItemValueProperty(const FName& Name) const
{
	if (const FMDFastBindingItem* Item = BindingItems.FindByKey(Name))
	{
		if (Item->Value != nullptr)
		{
			return Item->Value->GetOutputProperty();
		}
	}

	return nullptr;
}

TTuple<const FProperty*, void*> UMDFastBindingObject::GetBindingItemValue(UObject* SourceObject,
                                                                          const FName& Name) const
{
	if (const FMDFastBindingItem* Item = BindingItems.FindByKey(Name))
	{
		if (Item->Value != nullptr)
		{
			return Item->Value->GetValue(SourceObject);
		}
	}
	
	return {};
}

TTuple<const FProperty*, void*> UMDFastBindingObject::GetBindingItemValue(UObject* SourceObject, int32 Index) const
{
	if (BindingItems.IsValidIndex(Index))
	{
		if (BindingItems[Index].Value != nullptr)
		{
			return BindingItems[Index].Value->GetValue(SourceObject);
		}
	}
	
	return {};
}

#if WITH_EDITOR
EDataValidationResult UMDFastBindingObject::IsDataValid(TArray<FText>& ValidationErrors)
{
	for (const FMDFastBindingItem& BindingItem : BindingItems)
	{
		if (!BindingItem.ItemProperty.IsValid())
		{
			ValidationErrors.Add(FText::Format(LOCTEXT("BindingItemNullTypeError", "Pin '{0}' is missing an expected type"), FText::FromName(BindingItem.ItemName)));
			return EDataValidationResult::Invalid;
		}
		
		if (!BindingItem.bAllowNullValue && BindingItem.Value == nullptr)
		{
			ValidationErrors.Add(FText::Format(LOCTEXT("NullBindingItemValueError", "Pin '{0}' is empty"), FText::FromName(BindingItem.ItemName)));
			return EDataValidationResult::Invalid;
		}

		if (BindingItem.Value != nullptr)
		{
			const FProperty* OutputProp = BindingItem.Value->GetOutputProperty();
			if (OutputProp == nullptr)
			{
				ValidationErrors.Add(FText::Format(LOCTEXT("NullBindingItemValueTypeError", "Node '{0}' connected to Pin '{1}' has null output type"), BindingItem.Value->GetDisplayName(), FText::FromName(BindingItem.ItemName)));
				return EDataValidationResult::Invalid;
			}

			// TODO - Converters
			if (!OutputProp->SameType(BindingItem.ItemProperty.Get()))
			{
				ValidationErrors.Add(FText::Format(LOCTEXT("BindingItemTypeMismatchError", "Pin '{0}' expects type '{1}' but Value '{2}' has type '{3}'")
					, FText::FromName(BindingItem.ItemName)
					, FText::FromString(BindingItem.ItemProperty->GetCPPType())
					, BindingItem.Value->GetDisplayName()
					, FText::FromString(OutputProp->GetCPPType())));
				return EDataValidationResult::Invalid;
			}
		}
	}

	return EDataValidationResult::Valid;
}

void UMDFastBindingObject::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	SetupBindingItems();
}
#endif

#if WITH_EDITORONLY_DATA
FText UMDFastBindingObject::GetDisplayName() const
{
	return DevName.IsEmptyOrWhitespace() ? GetClass()->GetDisplayNameText() : DevName;
}

FText UMDFastBindingObject::GetToolTipText() const
{
	return GetClass()->GetToolTipText();
}

const FMDFastBindingItem* UMDFastBindingObject::FindBindingItem(const FName& ItemName) const
{
	return BindingItems.FindByKey(ItemName);
}

UMDFastBindingValueBase* UMDFastBindingObject::SetBindingItem(const FName& ItemName,
	TSubclassOf<UMDFastBindingValueBase> ValueClass)
{
	if (FMDFastBindingItem* BindingItem = BindingItems.FindByKey(ItemName))
	{
		if (UMDFastBindingValueBase* NewValue = NewObject<UMDFastBindingValueBase>(this, ValueClass, NAME_None, RF_Public | RF_Transactional))
		{
			BindingItem->Value = NewValue;
			return NewValue;
		}
	}

	return nullptr;
}

void UMDFastBindingObject::ClearBindingItemValue(const FName& ItemName)
{
	if (FMDFastBindingItem* BindingItem = BindingItems.FindByKey(ItemName))
	{
		BindingItem->Value = nullptr;
	}
}
#endif

#undef LOCTEXT_NAMESPACE
