#pragma once

#include "SPinValueInspector.h"
#include "UObject/WeakFieldPtr.h"


struct FMDFastBindingItem;
class UMDFastBindingInstance;
class UMDFastBindingObject;


class FMDFastBindingDebugLineItemBase : public FDebugLineItem
{
public:
	virtual bool CanHaveChildren() override { return true; }

	virtual bool HasChildren() const override;

	virtual void GatherChildrenBase(TArray<FDebugTreeItemPtr>& OutChildren, const FString& InSearchString, bool bRespectSearch) override;

	virtual void UpdateCachedChildren() const = 0;

protected:
	using FDebugLineItem::FDebugLineItem;

	mutable TOptional<TArray<FDebugTreeItemPtr>> CachedChildren;
};


class FMDFastBindingWatchedObjectNodeLineItem : public FMDFastBindingDebugLineItemBase
{
public:
	FMDFastBindingWatchedObjectNodeLineItem(UMDFastBindingObject* Object)
		: FMDFastBindingDebugLineItemBase(EDebugLineType::DLT_Parent)
		, WatchedObjectPtr(Object)
	{}

	void RefreshWatchedObject(UMDFastBindingObject* Object);

	virtual void UpdateCachedChildren() const override;

protected:
	virtual bool Compare(const FDebugLineItem* BaseOther) const override
	{
		const FMDFastBindingWatchedObjectNodeLineItem* Other = static_cast<const FMDFastBindingWatchedObjectNodeLineItem*>(BaseOther);
		return WatchedObjectPtr == Other->WatchedObjectPtr;
	}

	virtual uint32 GetHash() override
	{
		return GetTypeHash(WatchedObjectPtr.Get());
	}

	virtual FDebugLineItem* Duplicate() const override
	{
		return new FMDFastBindingWatchedObjectNodeLineItem(WatchedObjectPtr.Get());
	}

	virtual UObject* GetParentObject() override;

	virtual void ExtendContextMenu(class FMenuBuilder& MenuBuilder, bool bInDebuggerTab) override;

	virtual FText GetDisplayName() const override;
	virtual FText GetDescription() const override;

private:
	TWeakObjectPtr<UMDFastBindingObject> WatchedObjectPtr;
	mutable TMap<FName, FDebugTreeItemPtr> CachedPins;
	mutable bool bIsObjectDirty = true;
};


class FMDFastBindingDebugLineItem : public FMDFastBindingDebugLineItemBase
{
public:
	virtual TSharedRef<SWidget> GetNameIcon() override;

	virtual TSharedRef<SWidget> GenerateValueWidget(TSharedPtr<FString> InSearchString) override;

protected:
	FMDFastBindingDebugLineItem()
		: FMDFastBindingDebugLineItemBase(EDebugLineType::DLT_Watch)
	{}

	virtual bool Compare(const FDebugLineItem* BaseOther) const override
	{
		const FMDFastBindingDebugLineItem* Other = static_cast<const FMDFastBindingDebugLineItem*>(BaseOther);
		return GetPropertyInstance() == Other->GetPropertyInstance();
	}

	virtual uint32 GetHash() override
	{
		const TTuple<const FProperty*, void*> Instance = GetPropertyInstance();
		return HashCombine(GetTypeHash(Instance.Key), GetTypeHash(Instance.Value));
	}

	virtual TTuple<const FProperty*, void*> GetPropertyInstance() const = 0;

	virtual const FProperty* GetItemProperty() const;

	virtual FText GetDisplayValue() const;

	virtual void UpdateCachedChildren() const override;

private:
	mutable TMap<FName, FDebugTreeItemPtr> CachedPropertyItems;
};


class FMDFastBindingItemDebugLineItem : public FMDFastBindingDebugLineItem
{
public:
	FMDFastBindingItemDebugLineItem(UMDFastBindingObject* DebugObject, const FName& InItemName)
		: DebugObjectPtr(DebugObject)
		, ItemName(InItemName)
	{
	}

	void RefreshDebugObject(UMDFastBindingObject* DebugObject);

	virtual FText GetDisplayName() const override
	{
		return FText::FromName(ItemName);
	}

	const FMDFastBindingItem* GetBindingItem() const;

	virtual const FProperty* GetItemProperty() const override;

	const FName& GetItemName() const { return ItemName; }

protected:
	virtual FDebugLineItem* Duplicate() const override
	{
		return new FMDFastBindingItemDebugLineItem(DebugObjectPtr.Get(), ItemName);
	}

	virtual TTuple<const FProperty*, void*> GetPropertyInstance() const override;

	virtual UObject* GetParentObject() override;

	virtual void ExtendContextMenu(class FMenuBuilder& MenuBuilder, bool bInDebuggerTab) override;

private:
	TWeakObjectPtr<UMDFastBindingObject> DebugObjectPtr;
	FName ItemName;
};


class FMDFastBindingPropertyDebugLineItem : public FMDFastBindingDebugLineItem
{
public:
	FMDFastBindingPropertyDebugLineItem(const FProperty* Property, void* InValuePtr, const FText& InDisplayNameOverride = FText::GetEmpty())
		: PropertyPtr(Property)
		, ValuePtr(InValuePtr)
		, DisplayNameOverride(InDisplayNameOverride)
	{
	}

	virtual FText GetDisplayName() const override
	{
		if (!DisplayNameOverride.IsEmptyOrWhitespace())
		{
			return DisplayNameOverride;
		}

		if (const FProperty* Property = PropertyPtr.Get())
		{
			return Property->GetDisplayNameText();
		}

		return INVTEXT("[Invalid]");
	}

	void* GetValuePtr() const { return ValuePtr; }

	void UpdateValuePtr(void* InValuePtr);

protected:
	virtual FDebugLineItem* Duplicate() const override
	{
		return new FMDFastBindingPropertyDebugLineItem(PropertyPtr.Get(), ValuePtr, DisplayNameOverride);
	}

	virtual TTuple<const FProperty*, void*> GetPropertyInstance() const override;

private:
	TWeakFieldPtr<const FProperty> PropertyPtr;
	void* ValuePtr = nullptr;
	FText DisplayNameOverride;
};


class SMDFastBindingPinValueInspector : public SPinValueInspector
{
public:
	using SPinValueInspector::Construct;

	void SetReferences(UEdGraphPin* Pin, UMDFastBindingObject* DebugObject);

	bool Matches(UEdGraphPin* Pin, UMDFastBindingObject* DebugObject) const;

protected:
	virtual void PopulateTreeView() override;

	// We can't support search since that requires inheriting from FLineItemWithChildren but that's private
	virtual EVisibility GetSearchFilterVisibility() const override { return EVisibility::Collapsed; }

	FName PinName;

	TWeakObjectPtr<UMDFastBindingObject> DebugObjectPtr;
};


class MDFASTBINDINGBLUEPRINT_API SMDFastBindingWatchList : public SPinValueInspector
{
public:
	using SPinValueInspector::Construct;

	void SetReferences(UMDFastBindingInstance* InCDOBinding, UMDFastBindingInstance* InDebugBinding);

	void RefreshList();

protected:
	virtual void PopulateTreeView() override;
	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	// We can't support search since that requires inheriting from FLineItemWithChildren but that's private
	virtual EVisibility GetSearchFilterVisibility() const override { return EVisibility::Collapsed; }

	TWeakObjectPtr<UMDFastBindingInstance> CDOBinding;
	TWeakObjectPtr<UMDFastBindingInstance> DebugBinding;

private:
	TMap<FGuid, TSharedPtr<FMDFastBindingWatchedObjectNodeLineItem>> TreeItems;
	bool bIsDebugging = false;
};
