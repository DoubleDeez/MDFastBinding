#pragma once

#include "MDFastBindingFieldPath.h"
#include "MDFastBindingValueBase.h"
#include "MDFastBindingValue_Property.generated.h"

/**
 * Retrieve the value of a property or const function
 */
UCLASS(meta = (DisplayName = "Read a Property"))
class MDFASTBINDING_API UMDFastBindingValue_Property : public UMDFastBindingValueBase
{
	GENERATED_BODY()

public:
	UMDFastBindingValue_Property();

	virtual const FProperty* GetOutputProperty() override;

#if WITH_EDITORONLY_DATA
	virtual bool DoesBindingItemDefaultToSelf(const FName& InItemName) const override;
	virtual FText GetDisplayName() override;
#endif
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(TArray<FText>& ValidationErrors) override;

	virtual void OnVariableRenamed(UClass* VariableClass, const FName& OldVariableName, const FName& NewVariableName) override;

	void SetFieldPath(const TArray<FFieldVariant>& Path);
	const TArray<FFieldVariant>& GetFieldPath();
#endif

protected:
	virtual TTuple<const FProperty*, void*> GetValue_Internal(UObject* SourceObject) override;
	virtual UObject* GetPropertyOwner(UObject* SourceObject);
	virtual UStruct* GetPropertyOwnerStruct();

	virtual const FProperty* GetPathRootProperty() const { return nullptr; }

	virtual void SetupBindingItems() override;

	virtual void PostInitProperties() override;

	FFieldVariant GetLeafField();

	// Path to the property you want to get
	UPROPERTY(EditDefaultsOnly, Category = "Binding")
	FMDFastBindingFieldPath PropertyPath;
};
