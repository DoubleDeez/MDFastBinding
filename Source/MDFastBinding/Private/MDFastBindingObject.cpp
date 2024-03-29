#include "MDFastBindingObject.h"

#include "MDFastBinding.h"
#include "MDFastBindingContainer.h"
#include "MDFastBindingHelpers.h"
#include "MDFastBindingInstance.h"
#include "BindingValues/MDFastBindingValueBase.h"
#include "UObject/ObjectSaveContext.h"
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

TTuple<const FProperty*, void*> FMDFastBindingItem::GetValue(UObject* SourceObject, bool& OutDidUpdate)
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

	{
		const FProperty* EffectiveItemProp = ItemProperty.IsValid() ? ItemProperty.Get() : UMDFastBindingProperties::GetObjectProperty();
		if (AllocatedDefaultValue != nullptr)
		{
			return TTuple<const FProperty*, void*>{ EffectiveItemProp, AllocatedDefaultValue };
		}

		if (IsSelfPin() || IsWorldContextPin())
		{
			bHasRetrievedDefaultValue = true;
			UObject** SourceObjectPtr = &SourceObject;
			AllocatedDefaultValue = FMemory::Malloc(EffectiveItemProp->GetSize(), EffectiveItemProp->GetMinAlignment());
			EffectiveItemProp->InitializeValue(AllocatedDefaultValue);
			EffectiveItemProp->CopyCompleteValue(AllocatedDefaultValue, SourceObjectPtr);
			OutDidUpdate = true;

			return TTuple<const FProperty*, void*>{ EffectiveItemProp, AllocatedDefaultValue };
		}
	}

	const FProperty* ItemProp = ItemProperty.Get();
	if (ItemProp == nullptr)
	{
		return {};
	}

	OutDidUpdate = !HasRetrievedDefaultValue();
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
	else if (ItemProp->IsA<FTextProperty>())
	{
		bHasRetrievedDefaultValue = true;
		return TTuple<const FProperty*, void*>{ ItemProp, &DefaultText };
	}
	else if (const FObjectPropertyBase* ObjectProp = CastField<const FObjectPropertyBase>(ItemProp))
	{
		bHasRetrievedDefaultValue = true;
		AllocatedDefaultValue = FMemory::Malloc(ObjectProp->GetSize(), ObjectProp->GetMinAlignment());
		ObjectProp->InitializeValue(AllocatedDefaultValue);
		ObjectProp->SetObjectPropertyValue(AllocatedDefaultValue, DefaultObject);
		return TTuple<const FProperty*, void*>{ ObjectProp, AllocatedDefaultValue };
	}
	else if (!DefaultString.IsEmpty())
	{
		bHasRetrievedDefaultValue = true;
		AllocatedDefaultValue = FMemory::Malloc(ItemProp->GetSize(), ItemProp->GetMinAlignment());
		ItemProp->InitializeValue(AllocatedDefaultValue);
		ItemProp->ImportText_Direct(*DefaultString, AllocatedDefaultValue, nullptr, PPF_None);
		return TTuple<const FProperty*, void*>{ ItemProp, AllocatedDefaultValue };
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

const FName& UMDFastBindingObject::FindOrCreateExtendableItemName(const FName& Base, int32 Index)
{
	static TMap<TTuple<FName, int32>, FName> ItemNameMap;

	FName& Result = ItemNameMap.FindOrAdd(TTuple<FName, int32>(Base, Index));

	if (Result.IsNone())
	{
		Result = *FString::Printf(TEXT("%s %d"), *Base.ToString(), Index);
	}

	return Result;
}

void UMDFastBindingObject::PreSave(FObjectPreSaveContext SaveContext)
{
	Super::PreSave(SaveContext);

#if WITH_EDITORONLY_DATA
	// Make sure editor-only meta data is copied over when saving
	for (FMDFastBindingItem& BindingItem : BindingItems)
	{
		BindingItem.bIsSelfPin = DoesBindingItemDefaultToSelf(BindingItem.ItemName);
		BindingItem.bIsWorldContextPin = IsBindingItemWorldContextObject(BindingItem.ItemName);
	}
#endif
}

UClass* UMDFastBindingObject::GetBindingOwnerClass() const
{
#if !WITH_EDITOR
	if (BindingOwnerClass.IsValid())
	{
		return BindingOwnerClass.Get();
	}
#endif

	const UObject* Object = this;
	while (Object != nullptr)
	{
		if (const UMDFastBindingContainer* Container = Cast<UMDFastBindingContainer>(Object))
		{
			UClass* Class = Container->GetBindingOwnerClass();
			BindingOwnerClass = Class;
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
	if (UpdateType == EMDFastBindingUpdateType::Always)
	{
		return true;
	}

	if (UpdateType == EMDFastBindingUpdateType::EventBased && bIsObjectDirty)
	{
		// If dirty and event based, then we must update
		return true;
	}

	if (UpdateType == EMDFastBindingUpdateType::Once)
	{
		// Assume the child classes check for this
		return false;
	}

	// TODO - Optimize - Cache whether downstream items could ever possibly need an update when this object doesn't
	for (const FMDFastBindingItem& Item : BindingItems)
	{
		if (Item.Value != nullptr && Item.Value->CheckCachedNeedsUpdate())
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
						Item->ItemName = FindOrCreateExtendableItemName(Item->ExtendablePinListNameBase, Item->ExtendablePinListIndex);
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

void UMDFastBindingObject::MarkObjectClean()
{
	bIsObjectDirty = false;
}

bool UMDFastBindingObject::CheckCachedNeedsUpdate() const
{
	if (!CachedNeedsUpdate.IsSet())
	{
		CachedNeedsUpdate = CheckNeedsUpdate();
	}

	return CachedNeedsUpdate.GetValue();
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

FMDFastBindingItem& UMDFastBindingObject::EnsureBindingItemExists(const FName& ItemName, const FProperty* ItemProperty, const FText& ItemDescription, bool bIsOptional)
{
	FMDFastBindingItem* BindingItem = BindingItems.FindByKey(ItemName);
	if (BindingItem == nullptr)
	{
		FMDFastBindingItem Item;
		Item.ItemName = ItemName;
		BindingItems.Add(MoveTemp(Item));
		BindingItem = BindingItems.FindByKey(ItemName);
	}

#if WITH_EDITORONLY_DATA
	// These are only set at editor time since they depend on editor-only meta data
	BindingItem->bIsSelfPin = DoesBindingItemDefaultToSelf(ItemName);
	BindingItem->bIsWorldContextPin = IsBindingItemWorldContextObject(ItemName);
#endif
	BindingItem->bAllowNullValue = bIsOptional;
	BindingItem->ItemProperty = ItemProperty;
	BindingItem->ToolTip = ItemDescription;

	return *BindingItem;
}

FMDFastBindingItem& UMDFastBindingObject::EnsureExtendableBindingItemExists(const FName& NameBase, const FProperty* ItemProperty, const FText& ItemDescription, int32 ItemIndex, bool bIsOptional)
{
	const FName& ItemName = FindOrCreateExtendableItemName(NameBase, ItemIndex);
	FMDFastBindingItem& BindingItem = EnsureBindingItemExists(ItemName, ItemProperty, ItemDescription, bIsOptional);

	BindingItem.ExtendablePinListIndex = ItemIndex;
	BindingItem.ExtendablePinListNameBase = NameBase;

	return BindingItem;
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
	const int32 BindingIndex = BindingItems.IndexOfByKey(Name);
	return GetBindingItemValue(SourceObject, BindingIndex, OutDidUpdate);
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

UMDFastBindingValueBase* UMDFastBindingObject::FindBindingItemValue(const FName& ItemName) const
{
	if (const FMDFastBindingItem* Item = FindBindingItem(ItemName))
	{
		return Item->Value;
	}

	return nullptr;
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

void UMDFastBindingObject::GatherBindingValues(TArray<UMDFastBindingValueBase*>& OutValues) const
{
	for (const FMDFastBindingItem& BindingItem : BindingItems)
	{
		if (BindingItem.Value != nullptr)
		{
			OutValues.Add(BindingItem.Value);
			BindingItem.Value->GatherBindingValues(OutValues);
		}
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