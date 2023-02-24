#pragma once

#include "CoreMinimal.h"
#include "SPinValueInspector.h"
#include "UObject/WeakFieldPtr.h"


struct FMDFastBindingItem;
class UMDFastBindingObject;


class FMDFastBindingDebugLineItem : public FDebugLineItem
{
public:
	virtual bool CanHaveChildren() override { return true; }
	
	virtual bool HasChildren() const override;
	
	virtual TSharedRef<SWidget> GetNameIcon() override;

	virtual TSharedRef<SWidget> GenerateValueWidget(TSharedPtr<FString> InSearchString) override;
	
	virtual void GatherChildrenBase(TArray<FDebugTreeItemPtr>& OutChildren, const FString& InSearchString, bool bRespectSearch) override;

protected:
	FMDFastBindingDebugLineItem()
		: FDebugLineItem(EDebugLineType::DLT_Watch)
	{}

	// Compare this item to another of the same type
	virtual bool Compare(const FDebugLineItem* BaseOther) const override
	{
		const FMDFastBindingDebugLineItem* Other = static_cast<const FMDFastBindingDebugLineItem*>(BaseOther);
		return GetPropertyInstance() == Other->GetPropertyInstance();
	}

	// used for sets
	virtual uint32 GetHash() override
	{
		const TTuple<const FProperty*, void*> Instance = GetPropertyInstance();
		return HashCombine(GetTypeHash(Instance.Key), GetTypeHash(Instance.Value));
	}

	virtual TTuple<const FProperty*, void*> GetPropertyInstance() const = 0;
	
	virtual const FProperty* GetItemProperty() const;

	virtual FText GetDisplayValue() const;

	void UpdateCachedChildren() const;

private:
	mutable TOptional<TArray<FDebugTreeItemPtr>> CachedChildren;
};

class FMDFastBindingItemDebugLineItem : public FMDFastBindingDebugLineItem
{
public:
	FMDFastBindingItemDebugLineItem(UMDFastBindingObject* DebugObject, const FName& InItemName)
		: DebugObjectPtr(DebugObject)
		, ItemName(InItemName)
	{
	}

	virtual FText GetDisplayName() const override
	{
		return FText::FromName(ItemName);
	}

	const FMDFastBindingItem* GetBindingItem() const;

	virtual const FProperty* GetItemProperty() const override;

protected:
	virtual FDebugLineItem* Duplicate() const override
	{
		return new FMDFastBindingItemDebugLineItem(DebugObjectPtr.Get(), ItemName);
	}

	virtual TTuple<const FProperty*, void*> GetPropertyInstance() const override;

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

protected:
	virtual FDebugLineItem* Duplicate() const override
	{
		return new FMDFastBindingPropertyDebugLineItem(PropertyPtr.Get(), ValuePtr);
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
