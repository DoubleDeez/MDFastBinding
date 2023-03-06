#include "MDFastBindingObject.h"

#include "MDFastBinding.h"
#include "MDFastBindingContainer.h"
#include "MDFastBindingHelpers.h"
#include "MDFastBindingInstance.h"
#include "BindingValues/MDFastBindingValueBase.h"
#include "UObject/TextProperty.h"

#if WITH_EDITORONLY_DATA
#include "Misc/App.h"
#endif
#if WITH_EDITOR
#include "Widgets/Text/STextBlock.h"
#endif

#define LOCTEXT_NAMESPACE "MDFastBindingObject"


FMDFastBindingItem::~FMDFastBindingItem()
{
	if (AllocatedDefaultValue != nullptr)
	{
		FMemory::Free(AllocatedDefaultValue);
	}
}

TTuple<const FProperty*, void*> FMDFastBindingItem::GetValue(UObject* SourceObject, bool& OutDidUpdate, bool bAllowDefaults)
{
	OutDidUpdate = false;
	
	if (Value != nullptr)
	{
		const TTuple<const FProperty*, void*> Result = Value->GetValue(SourceObject, OutDidUpdate);
#if WITH_EDITORONLY_DATA
		if (OutDidUpdate)
		{
			LastUpdateTime = FApp::GetCurrentTime();
		}
#endif
		return Result;
	}
	
	if (!bAllowDefaults)
	{
		return {};
	}
	
	const FProperty* ItemProp = ItemProperty.Get();
	if (ItemProp == nullptr)
	{
		return {};
	}
	
	if (AllocatedDefaultValue != nullptr)
	{
		return TTuple<const FProperty*, void*>{ ItemProp, AllocatedDefaultValue };
	}

	OutDidUpdate = !bHasRetrievedDefaultValue;
#if WITH_EDITORONLY_DATA
	if (OutDidUpdate)
	{
		LastUpdateTime = FApp::GetCurrentTime();
	}
#endif
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
		ItemProp->ImportText_Direct(*DefaultString, AllocatedDefaultValue, nullptr, PPF_None);
		return TTuple<const FProperty*, void*>{ ItemProp, AllocatedDefaultValue };
	}
	else if (const FObjectPropertyBase* ObjectProp = CastField<const FObjectPropertyBase>(ItemProp))
	{
		bHasRetrievedDefaultValue = true;
		AllocatedDefaultValue = FMemory::Malloc(ObjectProp->GetSize(), ObjectProp->GetMinAlignment());
		ObjectProp->InitializeValue(AllocatedDefaultValue);
		ObjectProp->SetObjectPropertyValue(AllocatedDefaultValue, DefaultObject);
		return TTuple<const FProperty*, void*>{ ObjectProp, AllocatedDefaultValue };
	}
	else if (ItemProp->IsA<FTextProperty>())
	{
		bHasRetrievedDefaultValue = true;
		return TTuple<const FProperty*, void*>{ ItemProp, &DefaultText };
	}

	return {};
}

const FProperty* FMDFastBindingItem::ResolveOutputProperty() const
{
	if (const FProperty* Prop = ItemProperty.Get())
	{
		return Prop;
	}

	if (Value != nullptr)
	{
		return Value->GetOutputProperty();
	}

	return nullptr;
}

#if WITH_EDITOR
TTuple<const FProperty*, void*> FMDFastBindingItem::GetCachedValue() const
{
	if (Value != nullptr)
	{
		return Value->GetCachedValue();
	}
	
	if (const FProperty* ItemProp = ItemProperty.Get())
	{
		if (AllocatedDefaultValue != nullptr)
		{
			return TTuple<const FProperty*, void*>{ ItemProp, AllocatedDefaultValue };
		}
	}

	return {};
}
#endif

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

void UMDFastBindingObject::SetupBindingItems_Internal()
{
	SetupBindingItems();

	if (HasUserExtendablePinList())
	{
		for (int32 i = 0; i < ExtendablePinListCount; ++i)
		{
			SetupExtendablePinBindingItem(i);
		}
		
		// Remove items that go over ExtendablePinListCount
		for (int32 i = BindingItems.Num() - 1; i >= 0; --i)
		{
			if (BindingItems[i].ExtendablePinListIndex >= ExtendablePinListCount)
			{
#if WITH_EDITORONLY_DATA
				OrphanBindingItem(BindingItems[i].Value);
#endif
				BindingItems.RemoveAt(i);
			}
		}
	}
	else
	{
		// Remove items if we no longer have an extendable list
		for (int32 i = BindingItems.Num() - 1; i >= 0; --i)
		{
			if (BindingItems[i].ExtendablePinListIndex != INDEX_NONE)
			{
	#if WITH_EDITORONLY_DATA
				OrphanBindingItem(BindingItems[i].Value);
	#endif
				BindingItems.RemoveAt(i);
			}
		}

		ExtendablePinListCount = 0;
	}

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
		else if (UpdateType == EMDFastBindingUpdateType::EventBased && bIsObjectDirty)
		{
			// If dirty and event based, then we must update
			return true;
		}
		else if (UpdateType == EMDFastBindingUpdateType::IfUpdatesNeeded || UpdateType == EMDFastBindingUpdateType::EventBased)
		{
			// Event based when not dirty acts as `IfUpdatesNeeded` to keep their binding items up to date
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

void UMDFastBindingObject::RemoveExtendablePinBindingItem(int32 ItemIndex)
{
	bool bDidRemove = false;
	const int32 OldCount = ExtendablePinListCount;
	for (int32 i = BindingItems.Num() - 1; i >= 0; --i)
	{
		if (BindingItems[i].ExtendablePinListIndex == ItemIndex)
		{
#if WITH_EDITORONLY_DATA
			OrphanBindingItem(BindingItems[i].Value);
#endif
			BindingItems.RemoveAt(i);
			bDidRemove = true;
		}
	}

	if (bDidRemove)
	{
		--ExtendablePinListCount;

		// Rename items above ItemIndex
		int32 AccumulatedGap = 0;
		for (int32 i = 0; i < OldCount; ++i)
		{
			auto Pred = [i](const FMDFastBindingItem& Item) { return Item.ExtendablePinListIndex == i; };
			if (BindingItems.ContainsByPredicate(Pred))
			{
				if (AccumulatedGap > 0)
				{
					// Offset all items at i by AccumulatedGap
					while (FMDFastBindingItem* Item = BindingItems.FindByPredicate(Pred))
					{
						Item->ExtendablePinListIndex -= AccumulatedGap;
						Item->ItemName = CreateExtendableItemName(Item->ExtendablePinListNameBase, Item->ExtendablePinListIndex);
					}
				}
			}
			else
			{
				++AccumulatedGap;
			}
		}
	}
}

void UMDFastBindingObject::MarkObjectDirty()
{
	check(UpdateType == EMDFastBindingUpdateType::EventBased);

	bIsObjectDirty = true;

	if (UMDFastBindingInstance* BindingInstance = GetOuterBinding())
	{
		BindingInstance->MarkBindingDirty();
	}
}

bool UMDFastBindingObject::DoesObjectRequireTick() const
{
	if (UpdateType == EMDFastBindingUpdateType::Always)
	{
		return true;
	}

	if (UpdateType == EMDFastBindingUpdateType::EventBased && bIsObjectDirty)
	{
		return true;
	}
	
	for (const FMDFastBindingItem& Item : BindingItems)
	{
		if (Item.Value != nullptr && Item.Value->DoesObjectRequireTick())
		{
			return true;
		}
	}
	
	return false;
}

const FMDFastBindingItem* UMDFastBindingObject::FindBindingItemWithValue(const UMDFastBindingValueBase* Value) const
{
	for (const FMDFastBindingItem& Item : BindingItems)
	{
		if (Item.Value == Value)
		{
			return &Item;
		}
	}

	return nullptr;
}

const FMDFastBindingItem* UMDFastBindingObject::FindBindingItem(const FName& ItemName) const
{
	return BindingItems.FindByKey(ItemName);
}

FMDFastBindingItem* UMDFastBindingObject::FindBindingItem(const FName& ItemName)
{
	return BindingItems.FindByKey(ItemName);
}

void UMDFastBindingObject::PostLoad()
{
	Super::PostLoad();

#if WITH_EDITORONLY_DATA
	if (!BindingObjectIdentifier.IsValid())
	{
		BindingObjectIdentifier = FGuid::NewGuid();
	}
#endif
	
	SetupBindingItems_Internal();
	
}

void UMDFastBindingObject::PostInitProperties()
{
	Super::PostInitProperties();

	SetupBindingItems_Internal();
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

void UMDFastBindingObject::EnsureExtendableBindingItemExists(const FName& NameBase, const FProperty* ItemProperty, const FText& ItemDescription, int32 ItemIndex, bool bIsOptional)
{
	const FName ItemName = CreateExtendableItemName(NameBase, ItemIndex);
	EnsureBindingItemExists(ItemName, ItemProperty, ItemDescription, bIsOptional);

	FMDFastBindingItem* BindingItem = FindBindingItem(ItemName);
	check(BindingItem);
	BindingItem->ExtendablePinListIndex = ItemIndex;
	BindingItem->ExtendablePinListNameBase = NameBase;
}

const FProperty* UMDFastBindingObject::ResolveBindingItemProperty(const FName& Name) const
{
	if (const FMDFastBindingItem* Item = BindingItems.FindByKey(Name))
	{
		return Item->ResolveOutputProperty();
	}

	return nullptr;
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
		return Item->GetValue(SourceObject, OutDidUpdate, !DoesBindingItemDefaultToSelf(Name));
	}
	
	return {};
}

TTuple<const FProperty*, void*> UMDFastBindingObject::GetBindingItemValue(UObject* SourceObject, int32 Index, bool& OutDidUpdate)
{
	if (BindingItems.IsValidIndex(Index))
	{
		return BindingItems[Index].GetValue(SourceObject, OutDidUpdate, !DoesBindingItemDefaultToSelf(BindingItems[Index].ItemName));
	}
	
	return {};
}

#if WITH_EDITOR
EDataValidationResult UMDFastBindingObject::IsDataValid(TArray<FText>& ValidationErrors)
{
	SetupBindingItems_Internal();
	
	for (const FMDFastBindingItem& BindingItem : BindingItems)
	{
		const FProperty* ItemProp = BindingItem.ResolveOutputProperty();
		if (ItemProp == nullptr && !BindingItem.bAllowNullValue)
		{
			ValidationErrors.Add(FText::Format(LOCTEXT("BindingItemNullTypeError", "Wildcard Pin '{0}' is not connected"), FText::FromName(BindingItem.ItemName)));
			return EDataValidationResult::Invalid;
		}

		if (BindingItem.Value == nullptr && !BindingItem.bAllowNullValue)
		{
			if (ItemProp != nullptr && !ItemProp->IsA<FObjectPropertyBase>() && !ItemProp->IsA<FTextProperty>() && !ItemProp->IsA<FStrProperty>() && BindingItem.DefaultString.IsEmpty())
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

			if (!FMDFastBindingModule::CanSetProperty(BindingItem.ResolveOutputProperty(), OutputProp))
			{
				ValidationErrors.Add(FText::Format(LOCTEXT("BindingItemTypeMismatchError", "Pin '{0}' expects type '{1}' but Value '{2}' has type '{3}'")
					, FText::FromName(BindingItem.ItemName)
					, FText::FromString(FMDFastBindingHelpers::PropertyToString(*BindingItem.ResolveOutputProperty()))
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

	SetupBindingItems_Internal();
}

TSharedRef<SWidget> UMDFastBindingObject::CreateNodeHeaderWidget()
{
	return SNew(STextBlock).Text(TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateUObject(this, &UMDFastBindingObject::GetDisplayName)));
}

void UMDFastBindingObject::OnVariableRenamed(UClass* VariableClass, const FName& OldVariableName, const FName& NewVariableName)
{
	for (const FMDFastBindingItem& BindingItem : BindingItems)
	{
		if (BindingItem.Value != nullptr)
		{
			BindingItem.Value->OnVariableRenamed(VariableClass, OldVariableName, NewVariableName);
		}
	}
}
#endif

#if WITH_EDITORONLY_DATA
FText UMDFastBindingObject::GetDisplayName()
{
	return GetClass()->GetDisplayNameText();
}

FText UMDFastBindingObject::GetToolTipText()
{
	return GetClass()->GetToolTipText();
}

void UMDFastBindingObject::RemoveBindingItem(const FName& ItemName)
{
	OrphanBindingItem(ItemName);
	BindingItems.RemoveAll([ItemName](const FMDFastBindingItem& Item) { return Item.ItemName == ItemName; });
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
		Modify();
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
