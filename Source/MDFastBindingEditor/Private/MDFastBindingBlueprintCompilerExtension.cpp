#include "MDFastBindingBlueprintCompilerExtension.h"

#include "MDFastBindingHelpers.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "WidgetBlueprint.h"
#include "WidgetExtension/MDFastBindingWidgetBlueprintExtension.h"
#include "WidgetExtension/MDFastBindingWidgetClassExtension.h"

void UMDFastBindingBlueprintCompilerExtension::ProcessBlueprintCompiled(const FKismetCompilerContext& CompilationContext, const FBlueprintCompiledData& Data)
{
	Super::ProcessBlueprintCompiled(CompilationContext, Data);

	if (const UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(CompilationContext.Blueprint))
	{
		const bool bAlreadyHasExtension = WidgetBP->GetExtensions().ContainsByPredicate([](TObjectPtr<UBlueprintExtension> Extension)
		{
			return Extension != nullptr && Extension->IsA<UMDFastBindingWidgetBlueprintExtension>();
		});
		
		if (!bAlreadyHasExtension)
		{
			if (UWidgetBlueprintGeneratedClass* WidgetClass = Cast<UWidgetBlueprintGeneratedClass>(CompilationContext.NewClass))
			{
				if (FMDFastBindingHelpers::DoesClassHaveSuperClassBindings(Cast<UWidgetBlueprintGeneratedClass>(WidgetClass)))
				{
					// We can't add a blueprint extension since we're mid-compile here
					// So instead we want to add a Class extension, but we need a non-const FWidgetBlueprintCompilerContext to do that
					// which is why we end up with this gross-ness
					FWidgetBlueprintCompilerContext& WidgetCompilationContext = const_cast<FWidgetBlueprintCompilerContext&>(
						static_cast<const FWidgetBlueprintCompilerContext&>(CompilationContext)
					);

					UMDFastBindingWidgetClassExtension* BindingClassExtension = NewObject<UMDFastBindingWidgetClassExtension>(WidgetClass);
					WidgetCompilationContext.AddExtension(WidgetClass, BindingClassExtension);
				}
			}
		}
	}
}
