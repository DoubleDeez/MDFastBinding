#include "MDFastBindingBlueprint.h"

#include "BlueprintCompilationManager.h"
#include "WidgetBlueprint.h"
#include "BlueprintExtension/MDFastBindingBlueprintCompilerExtension.h"

#define LOCTEXT_NAMESPACE "FMDFastBindingBlueprintModule"

void FMDFastBindingBlueprintModule::StartupModule()
{
	BlueprintCompilerExtension = NewObject<UMDFastBindingBlueprintCompilerExtension>();
	BlueprintCompilerExtension->AddToRoot();

	FBlueprintCompilationManager::RegisterCompilerExtension(UWidgetBlueprint::StaticClass(), BlueprintCompilerExtension);
}

void FMDFastBindingBlueprintModule::ShutdownModule()
{
	if (IsValid(BlueprintCompilerExtension))
	{
		BlueprintCompilerExtension->RemoveFromRoot();
		BlueprintCompilerExtension = nullptr;
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMDFastBindingBlueprintModule, MDFastBindingBlueprint)