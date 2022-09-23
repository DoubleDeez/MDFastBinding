#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

struct FPropertyAndParent;

/**
 * 
 */
class MDFASTBINDINGEDITOR_API FMDFastBindingObjectCustomization : public IDetailCustomization
{

public:

	static TSharedRef<IDetailCustomization> MakeInstance() { return MakeShared<FMDFastBindingObjectCustomization>(); }
	
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
	virtual void CustomizeDetails(const TSharedPtr<IDetailLayoutBuilder>& DetailBuilder) override;

private:
	bool IsPropertyReadOnly(const FPropertyAndParent& PropertyAndParent) const;

	TWeakPtr<IDetailLayoutBuilder> WeakDetailBuilder;
};
