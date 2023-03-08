#include "WidgetExtension/MDFastBindingWidgetBlueprintExtension.h"

#include "MDFastBindingContainer.h"
#include "WidgetExtension/MDFastBindingWidgetClassExtension.h"

void UMDFastBindingWidgetBlueprintExtension::SetBindingContainer(UMDFastBindingContainer* InContainer)
{
	check(InContainer && InContainer->GetOuter() == this);
	BindingContainer = InContainer;
	BindContainerOwnerDelegate();
}

void UMDFastBindingWidgetBlueprintExtension::PostLoad()
{
	Super::PostLoad();
	
	BindContainerOwnerDelegate();
}

void UMDFastBindingWidgetBlueprintExtension::HandleBeginCompilation(FWidgetBlueprintCompilerContext& InCreationContext)
{
	Super::HandleBeginCompilation(InCreationContext);

	CompilerContext = &InCreationContext;
}

void UMDFastBindingWidgetBlueprintExtension::HandleFinishCompilingClass(UWidgetBlueprintGeneratedClass* Class)
{
	Super::HandleFinishCompilingClass(Class);
	
	if (CompilerContext != nullptr && BindingContainer != nullptr && BindingContainer->GetBindings().Num() > 0)
	{
		UMDFastBindingWidgetClassExtension* BindingClass = NewObject<UMDFastBindingWidgetClassExtension>();
		BindingClass->SetBindingContainer(BindingContainer);

		// There's a chance we could perform some compile-time steps here that would improve runtime performance
		
		CompilerContext->AddExtension(Class, BindingClass);
	}
}

bool UMDFastBindingWidgetBlueprintExtension::HandleValidateGeneratedClass(UWidgetBlueprintGeneratedClass* Class)
{
	if (BindingContainer == nullptr)
	{
		return false;
	}

	TArray<FText> ValidationErrors;
	if (BindingContainer->IsDataValid(ValidationErrors) == EDataValidationResult::Invalid)
	{
		return false;
	}
	
	return Super::HandleValidateGeneratedClass(Class);
}

void UMDFastBindingWidgetBlueprintExtension::HandleEndCompilation()
{
	Super::HandleEndCompilation();

	CompilerContext = nullptr;
}

void UMDFastBindingWidgetBlueprintExtension::BindContainerOwnerDelegate()
{
	if (BindingContainer != nullptr)
	{
		BindingContainer->GetBindingOwnerClassDelegate.BindUObject(this, &UMDFastBindingWidgetBlueprintExtension::GetBindingOwnerClass);
	}
}

UClass* UMDFastBindingWidgetBlueprintExtension::GetBindingOwnerClass() const
{
	if (const UWidgetBlueprint* WidgetBP = GetWidgetBlueprint())
	{
		return WidgetBP->GeneratedClass;
	}
	
	return nullptr;
}
