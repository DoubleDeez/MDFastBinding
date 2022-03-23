#pragma once

#include "CoreMinimal.h"
#include "MDFastBindingValueBase.h"
#include "MDFastBindingValue_ContainerLength.generated.h"

/**
 * Returns the number of elements in an Array, Set, or Map
 */
UCLASS(meta = (DisplayName = "Length"))
class MDFASTBINDING_API UMDFastBindingValue_ContainerLength : public UMDFastBindingValueBase
{
	GENERATED_BODY()

public:
	virtual const FProperty* GetOutputProperty() override;

#if WITH_EDITORONLY_DATA
	virtual FText GetDisplayName() override;
#endif
#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(TArray<FText>& ValidationErrors) override;
#endif
	
protected:
	virtual TTuple<const FProperty*, void*> GetValue_Internal(UObject* SourceObject) override;
	virtual void SetupBindingItems() override;

private:
	UPROPERTY(Transient)
	int32 OutputValue = 0;

	const FProperty* Int32Prop = nullptr;

};
