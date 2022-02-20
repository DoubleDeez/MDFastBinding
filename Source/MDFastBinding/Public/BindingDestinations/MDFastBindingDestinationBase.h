#pragma once

#include "CoreMinimal.h"
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

#if WITH_EDITOR
	bool IsActive() const;
#endif

protected:
	virtual void InitializeDestination_Internal(UObject* SourceObject) {}
	virtual void UpdateDestination_Internal(UObject* SourceObject) {}
	virtual void TerminateDestination_Internal(UObject* SourceObject) {}
};
