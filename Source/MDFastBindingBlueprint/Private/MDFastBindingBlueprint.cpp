#include "MDFastBindingBlueprint.h"

#include "BlueprintCompilationManager.h"
#include "WidgetBlueprint.h"
#include "BlueprintExtension/MDFastBindingBlueprintCompilerExtension.h"

#define LOCTEXT_NAMESPACE "FMDFastBindingBlueprintModule"

void FMDFastBindingBlueprintModule::StartupModule()
{
	BlueprintCompilerExtensionPtr = NewObject<UMDFastBindingBlueprintCompilerExtension>();
	BlueprintCompilerExtensionPtr->AddToRoot();

	FBlueprintCompilationManager::RegisterCompilerExtension(UWidgetBlueprint::StaticClass(), BlueprintCompilerExtensionPtr.Get());
}

void FMDFastBindingBlueprintModule::ShutdownModule()
{
	if (UMDFastBindingBlueprintCompilerExtension* BlueprintCompilerExtension = BlueprintCompilerExtensionPtr.Get())
	{
		BlueprintCompilerExtension->RemoveFromRoot();
		BlueprintCompilerExtension = nullptr;
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMDFastBindingBlueprintModule, MDFastBindingBlueprint)