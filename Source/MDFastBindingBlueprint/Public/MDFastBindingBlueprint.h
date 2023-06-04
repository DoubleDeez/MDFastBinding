#pragma once

#include "Modules/ModuleManager.h"

class UMDFastBindingBlueprintCompilerExtension;

class FMDFastBindingBlueprintModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
	UMDFastBindingBlueprintCompilerExtension* BlueprintCompilerExtension = nullptr;
};
