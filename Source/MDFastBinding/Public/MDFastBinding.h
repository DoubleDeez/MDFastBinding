#pragma once

#include "Modules/ModuleInterface.h"
#include "Templates/SharedPointer.h"

class FProperty;
class IMDFastBindingPropertySetter;

class MDFASTBINDING_API FMDFastBindingModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static void AddPropertySetter(TSharedRef<IMDFastBindingPropertySetter> InPropertySetter);

	static bool CanSetProperty(const FProperty* DestinationProp, const FProperty* SourceProp);
	// Bypasses any Setter on the destination property
	static void SetPropertyDirectly(const FProperty* DestinationProp, void* DestinationValuePtr, const FProperty* SourceProp, const void* SourceValuePtr);
	// Respects the Setter on the destination property
	static void SetPropertyInContainer(const FProperty* DestinationProp, void* DestinationContainerPtr, const FProperty* SourceProp, const void* SourceValuePtr);

private:
	TArray<TSharedRef<IMDFastBindingPropertySetter>> PropertySetters;

};
