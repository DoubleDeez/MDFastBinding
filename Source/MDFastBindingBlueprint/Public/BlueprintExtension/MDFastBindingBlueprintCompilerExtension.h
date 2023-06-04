#pragma once

#include "BlueprintCompilerExtension.h"
#include "MDFastBindingBlueprintCompilerExtension.generated.h"

/**
 * Injects a UMDFastBindingWidgetClassExtension in generated blueprint classes with blueprints that inherit from blueprints with bindings
 */
UCLASS()
class MDFASTBINDINGBLUEPRINT_API UMDFastBindingBlueprintCompilerExtension : public UBlueprintCompilerExtension
{
	GENERATED_BODY()

protected:
	virtual void ProcessBlueprintCompiled(const FKismetCompilerContext& CompilationContext, const FBlueprintCompiledData& Data) override;

};
