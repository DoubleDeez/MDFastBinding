#pragma once

#include "MDFastBindingOwnerInterface.h"
#include "Extensions/WidgetBlueprintGeneratedClassExtension.h"
#include "MDFastBindingWidgetClassExtension.generated.h"

class UMDFastBindingContainer;

/**
 * Holds the compiled BindingContainer and populates the binding extension on instances of the owning widget class
 */
UCLASS()
class MDFASTBINDING_API UMDFastBindingWidgetClassExtension : public UWidgetBlueprintGeneratedClassExtension, public IMDFastBindingOwnerInterface
{
	GENERATED_BODY()

public:
	virtual void Initialize(UUserWidget* UserWidget) override;

	virtual UClass* GetBindingOwnerClass() const override;

	bool HasBindings() const;

	UMDFastBindingContainer* GetBindingContainer() const { return BindingContainer; }

#if WITH_EDITOR
	virtual void Construct(UUserWidget* UserWidget) override;

	void SetBindingContainer(UMDFastBindingContainer* BPBindingContainer);
#endif

private:
	UPROPERTY(Instanced)
	TObjectPtr<UMDFastBindingContainer> BindingContainer = nullptr;
};
