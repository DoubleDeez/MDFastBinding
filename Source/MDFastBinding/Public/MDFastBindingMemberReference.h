#pragma once

#include "CoreMinimal.h"
#include "Engine/MemberReference.h"
#include "MDFastBindingMemberReference.generated.h"

/**
 * Specialized FMemberReference for use in MDFastBinding classes
 */
USTRUCT()
struct MDFASTBINDING_API FMDFastBindingMemberReference : public FMemberReference
{
	GENERATED_BODY()

public:
	UPROPERTY()
	bool bIsFunction = false;
	
};
