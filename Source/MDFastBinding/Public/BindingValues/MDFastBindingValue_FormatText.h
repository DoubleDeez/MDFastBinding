#pragma once

#include "CoreMinimal.h"
#include "MDFastBindingValueBase.h"
#include "MDFastBindingValue_FormatText.generated.h"

/**
 * Formats text based on the input format string and inputs
 */
UCLASS(meta = (DisplayName = "Format Text"))
class MDFASTBINDING_API UMDFastBindingValue_FormatText : public UMDFastBindingValueBase
{
	GENERATED_BODY()

public:
	virtual TTuple<const FProperty*, void*> GetValue(UObject* SourceObject) override;
	virtual const FProperty* GetOutputProperty() override;
	
protected:
	virtual void SetupBindingItems() override;
	
	UPROPERTY(EditAnywhere, Category = "Binding")
	FText FormatText = INVTEXT("{InputString}");

private:
	FTextFormat TextFormat;

	UPROPERTY(Transient)
	FText OutputValue;

	UPROPERTY(Transient)
	TArray<FName> Arguments;

	const FProperty* TextProp = nullptr;
	
	FFormatNamedArguments Args;
};
