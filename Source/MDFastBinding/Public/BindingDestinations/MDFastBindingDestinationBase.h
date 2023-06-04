#pragma once

#include "MDFastBindingObject.h"
#include "MDFastBindingDestinationBase.generated.h"

class UMDFastBindingValueBase;

/**
 *
 */
UCLASS(Abstract, DefaultToInstanced, EditInlineNew)
class MDFASTBINDING_API UMDFastBindingDestinationBase : public UMDFastBindingObject
{
	GENERATED_BODY()

public:
	void InitializeDestination(UObject* SourceObject);
	void UpdateDestination(UObject* SourceObject);
	void TerminateDestination(UObject* SourceObject);

	virtual bool CheckNeedsUpdate() const override;

	virtual bool DoesObjectRequireTick() const override;

#if WITH_EDITOR
	bool IsActive() const;
#endif

protected:
	virtual void InitializeDestination_Internal(UObject* SourceObject) {}
	virtual void UpdateDestination_Internal(UObject* SourceObject) {}
	virtual void TerminateDestination_Internal(UObject* SourceObject) {}

	UPROPERTY(Transient)
	bool bHasEverUpdated = false;
};
