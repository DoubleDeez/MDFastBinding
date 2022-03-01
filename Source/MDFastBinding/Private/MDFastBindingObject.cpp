#include "MDFastBindingObject.h"

#include "MDFastBinding.h"
#include "MDFastBindingContainer.h"
#include "MDFastBindingHelpers.h"
#include "MDFastBindingInstance.h"
#include "BindingValues/MDFastBindingValueBase.h"
#include "UObject/TextProperty.h"

#define LOCTEXT_NAMESPACE "MDFastBindingObject"

FMDFastBindingItem::~FMDFastBindingItem()
{
	if (AllocatedDefaultValue != nullptr)
	{
		FMemory::Free(AllocatedDefaultValue);
	}
}

TTuple<const FProperty*, void*> FMDFastBindingItem::GetValue(UObject* SourceObject, bool& OutDidUpdate)
{
	OutDidUpdate = false;
	
	const FProperty* ItemProp = ItemProperty.Get();
	if (ItemProp == nullptr)
	{
		return {};
	}
	
	if (Value != nullptr)
	{
		return Value->GetValue(SourceObject, OutDidUpdate);
	}
	else if (AllocatedDefaultValue != nullptr)
	{
		return TTuple<const FProperty*, void*>{ ItemProp, AllocatedDefaultValue };
	}

	OutDidUpdate = !bHasRetrievedDefaultValue;
	if (ItemProp->IsA<FStrProperty>())
	{
		bHasRetrievedDefaultValue = true;
		return TTuple<const FProperty*, void*>{ ItemProp, &DefaultString };
	}
	else if (!DefaultString.IsEmpty())
	{
		bHasRetrievedDefaultValue = true;
		AllocatedDefaultValue = FMemory::Malloc(ItemProp->GetSize(), ItemProp->GetMinAlignment());
		ItemProp->InitializeValue(AllocatedDefaultValue);
		ItemProp->ImportText(*DefaultString, AllocatedDefaultValue, PPF_None, nullptr);
		return TTuple<const FProperty*, void*>{ ItemProp, AllocatedDefaultValue };
	}
	else if (ItemProp->IsA<FObjectPropertyBase>())
	{
		bHasRetrievedDefaultValue = true;
		return TTuple<const FProperty*, void*>{ ItemProp, &DefaultObject };
	}
	else if (ItemProp->IsA<FTextProperty>())
	{
		bHasRetrievedDefaultValue = true;
		return TTuple<const FProperty*, void*>{ ItemProp, &DefaultText };
	}

	return {};
}

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


UMDFastBindingInstance* UMDFastBindingObject::GetOuterBinding() const
{
#if !WITH_EDITOR
	if (OuterBinding.IsValid())
	{
		return OuterBinding.Get();
	}
#endif

	UObject* Object = GetOuter();
	while (Object != nullptr)
	{
		if (UMDFastBindingInstance* Binding = Cast<UMDFastBindingInstance>(Object))
		{
			OuterBinding = Binding;
			return Binding;
		}

		Object = Object->GetOuter();
	}

	return nullptr;
}

bool UMDFastBindingObject::CheckNeedsUpdate() const
{
	if (UpdateType != EMDFastBindingUpdateType::Always)
	{
		if (UpdateType == EMDFastBindingUpdateType::Once)
		{
			// Assume the child classes check for this
			return false;
		}
		else if (UpdateType == EMDFastBindingUpdateType::IfUpdatesNeeded)
		{
			for (const FMDFastBindingItem& Item : BindingItems)
			{
				if (Item.Value != nullptr && Item.Value->CheckNeedsUpdate())
				{
					return true;
				}
				else if (Item.Value == nullptr && !Item.HasRetrievedDefaultValue())
				{
					return true;
				}
			}

			return false;
		}
	}

	return true;
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
		FMDFastBindingItem Item;
		Item.ItemName = ItemName;
		BindingItems.Add(MoveTemp(Item));
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

TTuple<const FProperty*, void*> UMDFastBindingObject::GetBindingItemValue(UObject* SourceObject, const FName& Name, bool& OutDidUpdate)
{
	if (FMDFastBindingItem* Item = BindingItems.FindByKey(Name))
	{
		return Item->GetValue(SourceObject, OutDidUpdate);
	}
	
	return {};
}

TTuple<const FProperty*, void*> UMDFastBindingObject::GetBindingItemValue(UObject* SourceObject, int32 Index, bool& OutDidUpdate)
{
	if (BindingItems.IsValidIndex(Index))
	{
		return BindingItems[Index].GetValue(SourceObject, OutDidUpdate);
	}
	
	return {};
}

#if WITH_EDITOR
EDataValidationResult UMDFastBindingObject::IsDataValid(TArray<FText>& ValidationErrors)
{
	SetupBindingItems();
	
	for (const FMDFastBindingItem& BindingItem : BindingItems)
	{
		if (!BindingItem.ItemProperty.IsValid())
		{
			ValidationErrors.Add(FText::Format(LOCTEXT("BindingItemNullTypeError", "Pin '{0}' is missing an expected type"), FText::FromName(BindingItem.ItemName)));
			return EDataValidationResult::Invalid;
		}

		if (BindingItem.Value == nullptr && !BindingItem.bAllowNullValue)
		{
			const FProperty* ItemProp = BindingItem.ItemProperty.Get();
			if (!ItemProp->IsA<FObjectPropertyBase>() && !ItemProp->IsA<FTextProperty>() && !ItemProp->IsA<FStrProperty>() && BindingItem.DefaultString.IsEmpty())
			{
				ValidationErrors.Add(FText::Format(LOCTEXT("NullBindingItemValueError", "Pin '{0}' is empty"), FText::FromName(BindingItem.ItemName)));
				return EDataValidationResult::Invalid;
			}
		}
		else if (BindingItem.Value != nullptr)
		{
			const FProperty* OutputProp = BindingItem.Value->GetOutputProperty();
			if (OutputProp == nullptr)
			{
				ValidationErrors.Add(FText::Format(LOCTEXT("NullBindingItemValueTypeError", "Node '{0}' connected to Pin '{1}' has null output type"), BindingItem.Value->GetDisplayName(), FText::FromName(BindingItem.ItemName)));
				return EDataValidationResult::Invalid;
			}

			if (!FMDFastBindingModule::CanSetProperty(BindingItem.ItemProperty.Get(), OutputProp))
			{
				ValidationErrors.Add(FText::Format(LOCTEXT("BindingItemTypeMismatchError", "Pin '{0}' expects type '{1}' but Value '{2}' has type '{3}'")
					, FText::FromName(BindingItem.ItemName)
					, FText::FromString(FMDFastBindingHelpers::PropertyToString(*BindingItem.ItemProperty.Get()))
					, BindingItem.Value->GetDisplayName()
					, FText::FromString(FMDFastBindingHelpers::PropertyToString(*OutputProp))));
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
FText UMDFastBindingObject::GetDisplayName()
{
	return DevName.IsEmptyOrWhitespace() ? GetClass()->GetDisplayNameText() : DevName;
}

FText UMDFastBindingObject::GetToolTipText()
{
	return GetClass()->GetToolTipText();
}

const FMDFastBindingItem* UMDFastBindingObject::FindBindingItem(const FName& ItemName) const
{
	return BindingItems.FindByKey(ItemName);
}

FMDFastBindingItem* UMDFastBindingObject::FindBindingItem(const FName& ItemName)
{
	return BindingItems.FindByKey(ItemName);
}

UMDFastBindingValueBase* UMDFastBindingObject::SetBindingItem(const FName& ItemName, TSubclassOf<UMDFastBindingValueBase> ValueClass)
{
	return SetBindingItem_Internal(ItemName,  NewObject<UMDFastBindingValueBase>(this, ValueClass, NAME_None, RF_Public | RF_Transactional));
}

UMDFastBindingValueBase* UMDFastBindingObject::SetBindingItem(const FName& ItemName, UMDFastBindingValueBase* InValue)
{
	return SetBindingItem_Internal(ItemName, DuplicateObject(InValue, this));
}

UMDFastBindingValueBase* UMDFastBindingObject::SetBindingItem_Internal(const FName& ItemName, UMDFastBindingValueBase* InValue)
{
	if (FMDFastBindingItem* BindingItem = BindingItems.FindByKey(ItemName))
	{
		if (BindingItem->Value != nullptr)
		{
			OrphanBindingItem(BindingItem->Value);
		}
		
		BindingItem->Value = InValue;
		BindingItem->ClearDefaultValues();
		return InValue;
	}

	return nullptr;
}

void UMDFastBindingObject::ClearBindingItemValue(const FName& ItemName)
{
	if (FMDFastBindingItem* BindingItem = BindingItems.FindByKey(ItemName))
	{
		BindingItem->Value = nullptr;
		BindingItem->ClearDefaultValues();
	}
}

void UMDFastBindingObject::ClearBindingItemValuePtrs()
{
	for (FMDFastBindingItem& BindingItem : BindingItems)
	{
		BindingItem.Value = nullptr;
	}
}

void UMDFastBindingObject::OrphanBindingItem(const FName& ItemName)
{
	if (const FMDFastBindingItem* BindingItem = BindingItems.FindByKey(ItemName))
	{
		OrphanBindingItem(BindingItem->Value);
	}

	ClearBindingItemValue(ItemName);
}

void UMDFastBindingObject::OrphanBindingItem(UMDFastBindingValueBase* InValue)
{
	if (UMDFastBindingInstance* Binding = GetOuterBinding())
	{
		Binding->AddOrphan(InValue);
	}
}

void UMDFastBindingObject::OrphanAllBindingItems(const TSet<UObject*>& OrphanExclusionSet)
{
	for (FMDFastBindingItem& Item : BindingItems)
	{
		if (!OrphanExclusionSet.Contains(Item.Value))
		{
			OrphanBindingItem(Item.Value);
			Item.Value = nullptr;
		}
	}
}
#endif

#undef LOCTEXT_NAMESPACE
