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

	void SetBindingTickPolicy(UMDFastBindingInstance* Binding, bool bShouldTick);
	
	UClass* GetBindingOwnerClass() const;

	// Use this redirect the binding owner from the actual outer of this UObject
	DECLARE_DELEGATE_RetVal(UClass*, FGetBindingOwnerClass);
	FGetBindingOwnerClass GetBindingOwnerClassDelegate;

// Editor only operations
#if WITH_EDITORONLY_DATA
	const TArray<UMDFastBindingInstance*>& GetBindings() const { return Bindings; }

	UMDFastBindingInstance* AddBinding();
	UMDFastBindingInstance* DuplicateBinding(UMDFastBindingInstance* InBinding);
	bool DeleteBinding(UMDFastBindingInstance* InBinding);

	void MoveBindingToIndex(UMDFastBindingInstance* InBinding, int32 Index);
#endif

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(TArray<FText>& ValidationErrors) override;
#endif

protected:
	UPROPERTY(Instanced)
	TArray<UMDFastBindingInstance*> Bindings;

	// Map binding indices to whether or not they should tick
	UPROPERTY(Transient)
	TMap<int32, bool> BindingTickPolicyLookUpMap;
};
