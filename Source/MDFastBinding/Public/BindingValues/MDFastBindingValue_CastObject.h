#pragma once

#include "MDFastBindingValueBase.h"
#include "UObject/Interface.h"

#include "MDFastBindingValue_CastObject.generated.h"

/**
 *
 */
UCLASS(meta = (DisplayName = "Cast Object"))
class MDFASTBINDING_API UMDFastBindingValue_CastObject : public UMDFastBindingValueBase
{
	GENERATED_BODY()

public:
	virtual const FProperty* GetOutputProperty() override;

#if WITH_EDITORONLY_DATA
	virtual FText GetDisplayName() override;
#endif

protected:
	virtual TTuple<const FProperty*, void*> GetValue_Internal(UObject* SourceObject) override;
	virtual void SetupBindingItems() override;

	UPROPERTY(EditDefaultsOnly, Category = "Binding", meta = (AllowAbstract))
	TSubclassOf<UObject> ObjectClass = UObject::StaticClass();

private:
	UPROPERTY(Transient)
	TObjectPtr<UObject> ObjectField = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UObject> ResultObject = nullptr;
	UPROPERTY(Transient)
	TScriptInterface<UInterface> ResultInterfaceField;
	FScriptInterface ResultInterface;

	const FProperty* ResultProp = nullptr;
};
