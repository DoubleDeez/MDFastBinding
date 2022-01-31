#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "UObject/WeakFieldPtr.h"
#include "MDFastBindingObject.generated.h"

class UMDFastBindingValueBase;

USTRUCT()
struct FMDFastBindingItem
{
	GENERATED_BODY()

public:
	UPROPERTY(VisibleAnywhere, Category = "Bindings")
	FName ItemName = NAME_None;
	
	UPROPERTY(EditDefaultsOnly, Instanced, Category = "Bindings")
	UMDFastBindingValueBase* Value = nullptr;

	FText ToolTip;

	TWeakFieldPtr<const FProperty> ItemProperty;

	bool bAllowNullValue = false;

	bool operator==(const FName& InName) const
	{
		return ItemName == InName;
	}
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

	virtual FText GetDisplayName() const;
	virtual FText GetToolTipText() const;

	const TArray<FMDFastBindingItem>& GetBindingItems() const { return BindingItems; }
	const FMDFastBindingItem* FindBindingItem(const FName& ItemName) const;
	UMDFastBindingValueBase* SetBindingItem(const FName& ItemName, TSubclassOf<UMDFastBindingValueBase> ValueClass);
	void ClearBindingItemValue(const FName& ItemName);
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
	TTuple<const FProperty*, void*> GetBindingItemValue(UObject* SourceObject, const FName& Name) const;
	TTuple<const FProperty*, void*> GetBindingItemValue(UObject* SourceObject, int32 Index) const;

	UPROPERTY()
	TArray<FMDFastBindingItem> BindingItems;

private:
	mutable TWeakObjectPtr<UClass> BindingOuterClass;
};
