#pragma once

#include "CoreMinimal.h"
#include "Extensions/UserWidgetExtension.h"
#include "MDFastBindingWidgetExtension.generated.h"

class UMDFastBindingContainer;

/**
 * A runtime BindingContainer instance for a user widget
 */
UCLASS()
class MDFASTBINDING_API UMDFastBindingWidgetExtension : public UUserWidgetExtension
{
	GENERATED_BODY()

	friend class UMDFastBindingWidgetClassExtension;
	
public:
	virtual void Construct() override;

	virtual void Destruct() override;

	virtual void Tick(const FGeometry& MyGeometry, float InDeltaTime) override;
	
	virtual bool RequiresTick() const override { return true; }

	// Call this to manually update bindings if you know source data might have changed after ticking but before painting
	void UpdateBindings();

#if WITH_EDITOR
	UMDFastBindingContainer* GetBindingContainer() const { return BindingContainer; }
#endif

protected:
	void SetBindingContainer(const UMDFastBindingContainer* CDOBindingContainer);
	
private:
	UClass* GetBindingOwnerClass() const;
	
	UPROPERTY(Instanced)
	TObjectPtr<UMDFastBindingContainer> BindingContainer = nullptr;
};
