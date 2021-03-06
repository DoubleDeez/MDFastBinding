#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleInterface.h"
#include "PropertySetters/IMDFastBindingPropertySetter.h"

class MDFASTBINDING_API FMDFastBindingModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static void AddPropertySetter(TSharedRef<IMDFastBindingPropertySetter> InPropertySetter);
	
	static bool CanSetProperty(const FProperty* DestinationProp, const FProperty* SourceProp);
	static void SetProperty(const FProperty* DestinationProp, void* DestinationValuePtr, const FProperty* SourceProp, const void* SourceValuePtr);

private:
	TArray<TSharedRef<IMDFastBindingPropertySetter>> PropertySetters;
	
};
