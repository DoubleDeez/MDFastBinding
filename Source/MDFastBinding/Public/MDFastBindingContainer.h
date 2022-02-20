#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MDFastBindingContainer.generated.h"

class UMDFastBindingInstance;

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
	const TArray<UMDFastBindingInstance*>& GetBindings() const { return Bindings; }

	UMDFastBindingInstance* AddBinding();
	UMDFastBindingInstance* DuplicateBinding(UMDFastBindingInstance* InBinding);
	bool DeleteBinding(UMDFastBindingInstance* InBinding);
#endif

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(TArray<FText>& ValidationErrors) override;
#endif

protected:
	UPROPERTY(Instanced)
	TArray<UMDFastBindingInstance*> Bindings;
};
