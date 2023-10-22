#pragma once

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
	void FixUpReference(UClass& OwnerClass) const;

	UPROPERTY(SaveGame)
	bool bIsFunction = false;

};
