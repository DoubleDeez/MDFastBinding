#pragma once

#include "CoreMinimal.h"
#include "MDFastBindingContainer.h"
#include "Blueprint/UserWidget.h"
#include "MDFastBindingUserWidget.generated.h"

class UMDFastBindingContainer;

/**
 * Base user widget class that has built-in Fast Binding functionality
 */
UCLASS(Abstract)
class MDFASTBINDING_API UMDFastBindingUserWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	
protected:
	UPROPERTY(Instanced, DuplicateTransient)
	UMDFastBindingContainer* Bindings = nullptr;
	
};
