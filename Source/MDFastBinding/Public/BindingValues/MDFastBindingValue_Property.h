#pragma once

#include "CoreMinimal.h"
#include "MDFastBindingFieldPath.h"
#include "MDFastBindingValueBase.h"
#include "MDFastBindingValue_Property.generated.h"

/**
 * Retrieve the value of a property or const function
 */
UCLASS(meta = (DisplayName = "Property"))
class MDFASTBINDING_API UMDFastBindingValue_Property : public UMDFastBindingValueBase
{
	GENERATED_BODY()

public:
	virtual TTuple<const FProperty*, void*> GetValue(UObject* SourceObject) override;
	virtual const FProperty* GetOutputProperty() override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(TArray<FText>& ValidationErrors) override;
#endif

protected:
	virtual UObject* GetPropertyOwner(UObject* SourceObject);
	virtual UClass* GetPropertyOwnerClass();
	
	virtual void SetupBindingItems() override;

	virtual void PostInitProperties() override;
	
	// Path to the property you want to get
	UPROPERTY(EditDefaultsOnly, Category = "Binding")
	FMDFastBindingFieldPath PropertyPath;

	UPROPERTY(Transient)
	UObject* ObjectProperty = nullptr;
};
