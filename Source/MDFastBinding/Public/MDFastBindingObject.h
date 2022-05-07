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
struct MDFASTBINDING_API FMDFastBindingItem
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

	UPROPERTY()
	int32 ExtendablePinListIndex = INDEX_NONE;

	UPROPERTY()
	FName ExtendablePinListNameBase = NAME_None;

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

	TTuple<const FProperty*, void*> GetValue(UObject* SourceObject, bool& OutDidUpdate, bool bAllowDefaults);

	bool HasRetrievedDefaultValue() const { return bHasRetrievedDefaultValue; }

	// Resolves wildcard binding items (where ItemProperty is null, the output property of Value is used instead)
	const FProperty* ResolveOutputProperty() const;

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
	
	void SetupBindingItems_Internal();

	virtual bool DoesBindingItemDefaultToSelf(const FName& InItemName) const { return false; }

	UMDFastBindingInstance* GetOuterBinding() const;

	virtual bool CheckNeedsUpdate() const;

	virtual bool HasUserExtendablePinList() const { return false; }

	void IncrementExtendablePinCount() { ++ExtendablePinListCount; }

	void RemoveExtendablePinBindingItem(int32 ItemIndex);
	
	const FMDFastBindingItem* FindBindingItemWithValue(const UMDFastBindingValueBase* Value) const;
	const FMDFastBindingItem* FindBindingItem(const FName& ItemName) const;
	FMDFastBindingItem* FindBindingItem(const FName& ItemName);
	
	static FName CreateExtendableItemName(const FName& Base, int32 Index)
	{
		return *FString::Printf(TEXT("%s %d"), *Base.ToString(), Index);
	}

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
	void RemoveBindingItem(const FName& ItemName);
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

	virtual TSharedRef<class SWidget> CreateNodeHeaderWidget();

	virtual void OnVariableRenamed(UClass* VariableClass, const FName& OldVariableName, const FName& NewVariableName);
#endif

protected:
	virtual void PostLoad() override;
	virtual void PostInitProperties() override;
	
	virtual void SetupBindingItems() {}

	virtual void SetupExtendablePinBindingItem(int32 ItemIndex) {}
	
	void EnsureBindingItemExists(const FName& ItemName, const FProperty* ItemProperty, const FText& ItemDescription, bool bIsOptional = false);
	void EnsureExtendableBindingItemExists(const FName& ItemName, const FProperty* ItemProperty, const FText& ItemDescription, int32 ItemIndex, bool bIsOptional = false);
	
	const FProperty* ResolveBindingItemProperty(const FName& Name) const;
	const FProperty* GetBindingItemValueProperty(const FName& Name) const;
	TTuple<const FProperty*, void*> GetBindingItemValue(UObject* SourceObject, const FName& Name, bool& OutDidUpdate);
	TTuple<const FProperty*, void*> GetBindingItemValue(UObject* SourceObject, int32 Index, bool& OutDidUpdate);

	UPROPERTY()
	TArray<FMDFastBindingItem> BindingItems;

	UPROPERTY()
	int32 ExtendablePinListCount = 0;
	
	// Values are cached, this setting determines when to grab a new value or use the cached value
	UPROPERTY(EditAnywhere, Category = "Performance")
	EMDFastBindingUpdateType UpdateType = EMDFastBindingUpdateType::IfUpdatesNeeded;

private:
	mutable TWeakObjectPtr<UClass> BindingOuterClass;
	mutable TWeakObjectPtr<UMDFastBindingInstance> OuterBinding;
};
