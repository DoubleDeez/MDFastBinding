#include "MDFastBinding.h"

#include "Modules/ModuleManager.h"
#include "PropertySetters/MDFastBindingPropertySetter_Colors.h"
#include "PropertySetters/MDFastBindingPropertySetter_Containers.h"
#include "PropertySetters/MDFastBindingPropertySetter_Numeric.h"
#include "PropertySetters/MDFastBindingPropertySetter_Objects.h"
#include "UObject/UnrealType.h"

#define LOCTEXT_NAMESPACE "FMDFastBindingModule"

void FMDFastBindingModule::StartupModule()
{
	AddPropertySetter(MakeShared<FMDFastBindingPropertySetter_Objects>());
	AddPropertySetter(MakeShared<FMDFastBindingPropertySetter_Containers>());
	AddPropertySetter(MakeShared<FMDFastBindingPropertySetter_Numeric>());
	AddPropertySetter(MakeShared<FMDFastBindingPropertySetter_Colors>());
}

void FMDFastBindingModule::ShutdownModule()
{
}

void FMDFastBindingModule::AddPropertySetter(TSharedRef<IMDFastBindingPropertySetter> InPropertySetter)
{
	FMDFastBindingModule& Module = FModuleManager::GetModuleChecked<FMDFastBindingModule>(TEXT("MDFastBinding"));
	Module.PropertySetters.Add(InPropertySetter);
}

bool FMDFastBindingModule::CanSetProperty(const FProperty* DestinationProp, const FProperty* SourceProp)
{
	if (SourceProp == nullptr || DestinationProp == nullptr)
	{
		return false;
	}

	FMDFastBindingModule& Module = FModuleManager::GetModuleChecked<FMDFastBindingModule>(TEXT("MDFastBinding"));
	for (const TSharedRef<IMDFastBindingPropertySetter>& Setter : Module.PropertySetters)
	{
		if (Setter->CanSetProperty(*DestinationProp, *SourceProp))
		{
			return true;
		}
	}

	// Fallback to same type check
	return SourceProp != nullptr && DestinationProp != nullptr && SourceProp->SameType(DestinationProp);
}

void FMDFastBindingModule::SetPropertyDirectly(const FProperty* DestinationProp, void* DestinationValuePtr, const FProperty* SourceProp, const void* SourceValuePtr)
{
	if (SourceProp == nullptr || SourceValuePtr == nullptr || DestinationProp == nullptr || DestinationValuePtr == nullptr)
	{
		return;
	}

	FMDFastBindingModule& Module = FModuleManager::GetModuleChecked<FMDFastBindingModule>(TEXT("MDFastBinding"));
	for (const TSharedRef<IMDFastBindingPropertySetter>& Setter : Module.PropertySetters)
	{
		if (Setter->CanSetProperty(*DestinationProp, *SourceProp))
		{
			Setter->SetPropertyDirectly(*DestinationProp, DestinationValuePtr, *SourceProp, SourceValuePtr);
			return;
		}
	}

	// Fallback to setting same type
	if (SourceProp->SameType(DestinationProp))
	{
		DestinationProp->CopyCompleteValue(DestinationValuePtr, SourceValuePtr);
	}
}

void FMDFastBindingModule::SetPropertyInContainer(const FProperty* DestinationProp, void* DestinationContainerPtr, const FProperty* SourceProp, const void* SourceValuePtr)
{
	if (SourceProp == nullptr || SourceValuePtr == nullptr || DestinationProp == nullptr || DestinationContainerPtr == nullptr)
	{
		return;
	}

	FMDFastBindingModule& Module = FModuleManager::GetModuleChecked<FMDFastBindingModule>(TEXT("MDFastBinding"));
	for (const TSharedRef<IMDFastBindingPropertySetter>& Setter : Module.PropertySetters)
	{
		if (Setter->CanSetProperty(*DestinationProp, *SourceProp))
		{
			Setter->SetPropertyInContainer(*DestinationProp, DestinationContainerPtr, *SourceProp, SourceValuePtr);
			return;
		}
	}

	// Fallback to setting same type
	if (SourceProp->SameType(DestinationProp))
	{
		DestinationProp->SetValue_InContainer(DestinationContainerPtr, SourceValuePtr);
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMDFastBindingModule, MDFastBinding)