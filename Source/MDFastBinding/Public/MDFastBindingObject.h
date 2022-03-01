#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/WeakFieldPtr.h"
#include "Templates/SubclassOf.h"
#include "MDFastBindingObject.generated.h"

class UMDFastBindingValueBase;
class UMDFastBindingInstance;


UENUM()
enum class EMDFastBindingUpdateType
{
	// Will always grab the latest value, regardless if inputs have changed
	Always,
	// Will only grab the latest value is any of the inputs have changed.
	// Some values treat this as "Always" (eg. Value_Property) since the only way to know if it changed is to get the value.
	IfUpdatesNeeded,
	// Will grab the latest value until it's successful, then reuses that value in future updates
	Once
};

// Represented as a pin in the binding editor graph
USTRUCT()
struct FMDFastBindingItem
{
	GENERATED_BODY()

public:
	~FMDFastBindingItem();
	
	UPROPERTY(VisibleAnywhere, Category = "Bindings")
	FName ItemName = NAME_None;
	
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Bindings")
	UMDFastBindingValueBase* Value = nullptr;

	UPROPERTY()
	FString DefaultString;

	UPROPERTY()
	FText DefaultText;

	UPROPERTY()
	UObject* DefaultObject = nullptr;

	FText ToolTip;

	TWeakFieldPtr<const FProperty> ItemProperty;

	bool bAllowNullValue = false;

	bool operator==(const FName& InName) const
	{
		return ItemName == InName;
	}

	void ClearDefaultValues()
	{
		DefaultString = {};
		DefaultText = {};
		DefaultObject = nullptr;
	}

	bool HasValue() const
	{
		return Value != nullptr || DefaultObject != nullptr || !DefaultString.IsEmpty() || !DefaultText.IsEmpty();
	}

	TTuple<const FProperty*, void*> GetValue(UObject* SourceObject, bool& OutDidUpdate);

	bool HasRetrievedDefaultValue() const { return bHasRetrievedDefaultValue; }

private:
	void* AllocatedDefaultValue = nullptr;

	UPROPERTY(Transient)
	bool bHasRetrievedDefaultValue = false;
};

/**
 * 
 */
UCLASS()
class MDFASTBINDING_API UMDFastBindingObject : public UObject
{
	GENERATED_BODY()

public:
	UClass* GetBindingOuterClass() const;
	
	virtual void SetupBindingItems() {}

	virtual bool DoesBindingItemDefaultToSelf(const FName& InItemName) const { return false; }

	UMDFastBindingInstance* GetOuterBinding() const;

	virtual bool CheckNeedsUpdate() const;

// Editor only operations
#if WITH_EDITORONLY_DATA
	UPROPERTY()
	FIntPoint NodePos = FIntPoint::ZeroValue;
	
	UPROPERTY()
	FText DevName;

	UPROPERTY()
	FString DevComment;
	
	UPROPERTY()
	bool bIsCommentBubbleVisible = false;

	virtual FText GetDisplayName();
	virtual FText GetToolTipText();

	const TArray<FMDFastBindingItem>& GetBindingItems() const { return BindingItems; }
	TArray<FMDFastBindingItem>& GetBindingItems() { return BindingItems; }
	const FMDFastBindingItem* FindBindingItem(const FName& ItemName) const;
	FMDFastBindingItem* FindBindingItem(const FName& ItemName);
	UMDFastBindingValueBase* SetBindingItem(const FName& ItemName, TSubclassOf<UMDFastBindingValueBase> ValueClass);
	UMDFastBindingValueBase* SetBindingItem(const FName& ItemName, UMDFastBindingValueBase* InValue);
	void ClearBindingItemValue(const FName& ItemName);
	void ClearBindingItemValuePtrs();
	
	void OrphanBindingItem(const FName& ItemName);
	void OrphanBindingItem(UMDFastBindingValueBase* InValue);
	void OrphanAllBindingItems(const TSet<UObject*>& OrphanExclusionSet);

private:
	UMDFastBindingValueBase* SetBindingItem_Internal(const FName& ItemName, UMDFastBindingValueBase* InValue);

public:
#endif

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(TArray<FText>& ValidationErrors) override;

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	virtual void PostLoad() override;
	virtual void PostInitProperties() override;

	void EnsureBindingItemExists(const FName& ItemName, const FProperty* ItemProperty, const FText& ItemDescription, bool bIsOptional = false);
	
	const FProperty* GetBindingItemValueProperty(const FName& Name) const;
	TTuple<const FProperty*, void*> GetBindingItemValue(UObject* SourceObject, const FName& Name, bool& OutDidUpdate);
	TTuple<const FProperty*, void*> GetBindingItemValue(UObject* SourceObject, int32 Index, bool& OutDidUpdate);

	UPROPERTY()
	TArray<FMDFastBindingItem> BindingItems;
	
	// Values are cached, this setting determines when to grab a new value or use the cached value
	UPROPERTY(EditAnywhere, Category = "Performance")
	EMDFastBindingUpdateType UpdateType = EMDFastBindingUpdateType::IfUpdatesNeeded;

private:
	mutable TWeakObjectPtr<UClass> BindingOuterClass;
	mutable TWeakObjectPtr<UMDFastBindingInstance> OuterBinding;
};
