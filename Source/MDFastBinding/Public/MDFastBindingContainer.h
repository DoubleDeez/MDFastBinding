#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MDFastBindingContainer.generated.h"

class UMDFastBindingDestinationBase;

/**
 * 
 */
UCLASS(DefaultToInstanced, EditInlineNew, meta = (DisplayName = "Binding Container"))
class MDFASTBINDING_API UMDFastBindingContainer : public UObject
{
	GENERATED_BODY()

public:
	void InitializeBindings(UObject* SourceObject);
	
	void UpdateBindings(UObject* SourceObject);

	void TerminateBindings(UObject* SourceObject);

// Editor only operations
#if WITH_EDITORONLY_DATA
	const TArray<UMDFastBindingDestinationBase*>& GetBindings() const { return Destinations; }

	UMDFastBindingDestinationBase* AddBinding(TSubclassOf<UMDFastBindingDestinationBase> BindingClass);
	UMDFastBindingDestinationBase* DuplicateBinding(UMDFastBindingDestinationBase* InBinding);
	bool DeleteBinding(UMDFastBindingDestinationBase* InBinding);
#endif

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(TArray<FText>& ValidationErrors) override;
#endif

protected:
	UPROPERTY(Instanced)
	TArray<UMDFastBindingDestinationBase*> Destinations;
};
