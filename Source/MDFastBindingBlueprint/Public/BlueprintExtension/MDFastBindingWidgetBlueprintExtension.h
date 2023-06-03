#pragma once

#include "CoreMinimal.h"
#include "MDFastBindingOwnerInterface.h"
#include "WidgetBlueprintExtension.h"
#include "MDFastBindingWidgetBlueprintExtension.generated.h"

class UMDFastBindingContainer;

/*
 * Holds the design time BindingContainer and compiles it to a MDFastBindingWidgetClassExtension
 */
UCLASS()
class MDFASTBINDINGBLUEPRINT_API UMDFastBindingWidgetBlueprintExtension : public UWidgetBlueprintExtension, public IMDFastBindingOwnerInterface
{
	GENERATED_BODY()

public:
	UMDFastBindingContainer* GetBindingContainer() const { return BindingContainer; }

	void SetBindingContainer(UMDFastBindingContainer* InContainer);

	bool DoesBlueprintOrSuperClassesHaveBindings() const;

	virtual UClass* GetBindingOwnerClass() const override;

#if WITH_EDITORONLY_DATA
	virtual void GetAllGraphs(TArray<UEdGraph*>& Graphs) const
#if defined(WITH_FASTBINDING_DIFFS) && WITH_FASTBINDING_DIFFS
	override
#endif
	;
#endif

protected:
	virtual void HandleBeginCompilation(FWidgetBlueprintCompilerContext& InCreationContext) override;
	virtual void HandleFinishCompilingClass(UWidgetBlueprintGeneratedClass* Class) override;
	virtual bool HandleValidateGeneratedClass(UWidgetBlueprintGeneratedClass* Class) override;
	virtual void HandleEndCompilation() override;

	UPROPERTY(Instanced)
	TObjectPtr<UMDFastBindingContainer> BindingContainer = nullptr;

private:
	// Temporary graphs that are pinned for use with in the diff tool
	UPROPERTY(Transient)
	mutable TArray<TObjectPtr<class UMDFastBindingGraph>> PinnedGraphs;

	FWidgetBlueprintCompilerContext* CompilerContext = nullptr;
};
