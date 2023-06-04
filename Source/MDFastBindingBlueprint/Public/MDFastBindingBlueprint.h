#pragma once

#include "Modules/ModuleManager.h"

#include "UObject/WeakObjectPtr.h"

class UMDFastBindingBlueprintCompilerExtension;

class FMDFastBindingBlueprintModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
	TWeakObjectPtr<UMDFastBindingBlueprintCompilerExtension> BlueprintCompilerExtensionPtr;
};
