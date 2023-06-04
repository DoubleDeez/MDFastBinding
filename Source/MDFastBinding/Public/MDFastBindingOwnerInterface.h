#pragma once

#include "UObject/Interface.h"
#include "MDFastBindingOwnerInterface.generated.h"

UINTERFACE()
class UMDFastBindingOwnerInterface : public UInterface
{
	GENERATED_BODY()
};

/**
 * Interface for objects that own binding containers that need to provide additional context
 */
class MDFASTBINDING_API IMDFastBindingOwnerInterface
{
	GENERATED_BODY()

public:
	virtual UClass* GetBindingOwnerClass() const = 0;
};
