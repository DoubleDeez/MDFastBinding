#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/WeakFieldPtr.h"
#include "MDFastBindingObject.generated.h"

class UMDFastBindingValueBase;
class UMDFastBindingInstance;

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

	TTuple<const FProperty*, void*> GetValue(UObject* SourceObject);

private:
	void* AllocatedDefaultValue = nullptr;
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
	const FMDFastBindingItem* FindBindingItem(const FName& ItemName) const;
	FMDFastBindingItem* FindBindingItem(const FName& ItemName);
	UMDFastBindingValueBase* SetBindingItem(const FName& ItemName, TSubclassOf<UMDFastBindingValueBase> ValueClass);
	UMDFastBindingValueBase* SetBindingItem(const FName& ItemName, UMDFastBindingValueBase* InValue);
	void ClearBindingItemValue(const FName& ItemName);
	
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
	TTuple<const FProperty*, void*> GetBindingItemValue(UObject* SourceObject, const FName& Name);
	TTuple<const FProperty*, void*> GetBindingItemValue(UObject* SourceObject, int32 Index);

	UPROPERTY()
	TArray<FMDFastBindingItem> BindingItems;

private:
	mutable TWeakObjectPtr<UClass> BindingOuterClass;
	mutable TWeakObjectPtr<UMDFastBindingInstance> OuterBinding;
};
