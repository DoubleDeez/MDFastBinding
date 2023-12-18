#pragma once

#include "MDFastBindingFieldPath.h"
#include "BindingDestinations/MDFastBindingDestinationBase.h"
#include "MDFastBindingDestination_Property.generated.h"

/**
 * Set the value of a property
 */
UCLASS(collapseCategories, meta=(DisplayName = "Set a Property"))
class MDFASTBINDING_API UMDFastBindingDestination_Property : public UMDFastBindingDestinationBase
{
	GENERATED_BODY()

public:
	UMDFastBindingDestination_Property();

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(TArray<FText>& ValidationErrors) override;

	virtual void OnVariableRenamed(UClass* VariableClass, const FName& OldVariableName, const FName& NewVariableName) override;

	void SetFieldPath(const TArray<FFieldVariant>& Path);
	TArray<FFieldVariant> GetFieldPath();
#endif

#if WITH_EDITORONLY_DATA
	virtual bool DoesBindingItemDefaultToSelf(const FName& InItemName) const override;

	virtual FText GetDisplayName() override;
#endif

protected:
	virtual void InitializeDestination_Internal(UObject* SourceObject) override;
	virtual void UpdateDestination_Internal(UObject* SourceObject) override;

	virtual void PostInitProperties() override;

	virtual UObject* GetPropertyOwner(UObject* SourceObject);
	virtual UStruct* GetPropertyOwnerStruct();

	virtual void SetupBindingItems() override;

	// Path to the property you want to set
	UPROPERTY(EditDefaultsOnly, Category = "Binding")
	FMDFastBindingFieldPath PropertyPath;

	UPROPERTY(Transient)
	UObject* ObjectProperty = nullptr;

	UPROPERTY(Transient)
	bool bNeedsUpdate = false;
};
