#pragma once

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

	bool HasBindings() const { return !Bindings.IsEmpty(); }

	bool DoesNeedTick() const { return TickingBindings.Contains(true); }

	UClass* GetBindingOwnerClass() const;

	UE_DEPRECATED(all, "GetBindingOwnerClassDelegate is deprecated, the binding's outer object should implement IMDFastBindingOwnerInterface instead")
	FSimpleDelegate GetBindingOwnerClassDelegate;

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

	// Array aligned with Bindings indicating whether or not to tick the binding of the same index
	TBitArray<> TickingBindings;

private:
	void UpdateNeedsTick();
};
