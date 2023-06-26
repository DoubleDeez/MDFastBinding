#pragma once

#include "MDFastBindingOwnerInterface.h"
#include "Extensions/UserWidgetExtension.h"
#include "MDFastBindingWidgetExtension.generated.h"

class UMDFastBindingContainer;

/**
 * A runtime BindingContainer instance for a user widget
 */
UCLASS()
class MDFASTBINDING_API UMDFastBindingWidgetExtension : public UUserWidgetExtension, public IMDFastBindingOwnerInterface
{
	GENERATED_BODY()

	friend class UMDFastBindingWidgetClassExtension;

public:
	virtual void Construct() override;

	virtual void Destruct() override;

	virtual void Tick(const FGeometry& MyGeometry, float InDeltaTime) override;

	virtual bool RequiresTick() const override;

	// Call this to manually update bindings if you know source data might have changed after ticking but before painting
	void UpdateBindings();

	virtual UClass* GetBindingOwnerClass() const override;

	void UpdateNeedsTick();

#if WITH_EDITOR
	UMDFastBindingContainer* GetBindingContainer() const { return BindingContainer; }
	const TArray<TObjectPtr<UMDFastBindingContainer>>& GetSuperBindingContainers() const { return SuperBindingContainers; }
#endif

protected:
	void SetBindingContainer(const UMDFastBindingContainer* CDOBindingContainer);
	void AddSuperBindingContainer(const UMDFastBindingContainer* SuperCDOBindingContainer);

private:
	UPROPERTY(Instanced)
	TObjectPtr<UMDFastBindingContainer> BindingContainer = nullptr;

	UPROPERTY(Instanced)
	TArray<TObjectPtr<UMDFastBindingContainer>> SuperBindingContainers;

	// Index 0 is BindingContainer, SuperBindingContainers starts from Index 1
	TBitArray<> TickingContainers;
};
