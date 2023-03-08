#pragma once

#include "CoreMinimal.h"
#include "Extensions/WidgetBlueprintGeneratedClassExtension.h"
#include "MDFastBindingWidgetClassExtension.generated.h"

class UMDFastBindingContainer;

/**
 * Holds the compiled BindingContainer and populates the binding extension on instances of the owning widget class
 */
UCLASS()
class MDFASTBINDING_API UMDFastBindingWidgetClassExtension : public UWidgetBlueprintGeneratedClassExtension
{
	GENERATED_BODY()

public:
	virtual void Initialize(UUserWidget* UserWidget) override;

#if WITH_EDITOR
	void SetBindingContainer(UMDFastBindingContainer* BPBindingContainer);
	UMDFastBindingContainer* GetBindingContainer() const { return BindingContainer; }
#endif

private:
	UPROPERTY(Instanced)
	TObjectPtr<UMDFastBindingContainer> BindingContainer = nullptr;
};
