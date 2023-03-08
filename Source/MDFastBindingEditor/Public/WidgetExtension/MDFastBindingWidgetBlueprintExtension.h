#pragma once

#include "CoreMinimal.h"
#include "WidgetBlueprintExtension.h"
#include "MDFastBindingWidgetBlueprintExtension.generated.h"

class UMDFastBindingContainer;

/*
 * Holds the design time BindingContainer and compiles it to a MDFastBindingWidgetClassExtension
 */
UCLASS()
class MDFASTBINDINGEDITOR_API UMDFastBindingWidgetBlueprintExtension : public UWidgetBlueprintExtension
{
	GENERATED_BODY()

public:
	UMDFastBindingContainer* GetBindingContainer() const { return BindingContainer; }

	void SetBindingContainer(UMDFastBindingContainer* InContainer);

	virtual void PostLoad() override;

protected:
	virtual void HandleBeginCompilation(FWidgetBlueprintCompilerContext& InCreationContext) override;
	virtual void HandleFinishCompilingClass(UWidgetBlueprintGeneratedClass* Class) override;
	virtual bool HandleValidateGeneratedClass(UWidgetBlueprintGeneratedClass* Class) override;
	virtual void HandleEndCompilation() override;

	UPROPERTY(Instanced)
	TObjectPtr<UMDFastBindingContainer> BindingContainer = nullptr;

private:
	void BindContainerOwnerDelegate();

	UClass* GetBindingOwnerClass() const;
	
	FWidgetBlueprintCompilerContext* CompilerContext = nullptr;
};
