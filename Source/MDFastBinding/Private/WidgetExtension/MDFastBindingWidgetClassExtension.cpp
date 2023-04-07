#include "WidgetExtension/MDFastBindingWidgetClassExtension.h"

#include "Blueprint/UserWidget.h"
#include "MDFastBindingContainer.h"
#include "MDFastBindingHelpers.h"
#include "WidgetExtension/MDFastBindingWidgetExtension.h"

void UMDFastBindingWidgetClassExtension::Initialize(UUserWidget* UserWidget)
{
	Super::Initialize(UserWidget);

	if (!IsValid(UserWidget))
	{
		return;
	}

	UWidgetBlueprintGeneratedClass* WidgetClass = Cast<UWidgetBlueprintGeneratedClass>(UserWidget->GetClass());
	if (!HasBindings() && !FMDFastBindingHelpers::DoesClassHaveSuperClassBindings(WidgetClass))
	{
		return;
	}

	UMDFastBindingWidgetExtension* SpawnedExtension = UserWidget->AddExtension<UMDFastBindingWidgetExtension>();
	if (HasBindings())
	{
		SpawnedExtension->SetBindingContainer(BindingContainer);
	}

	// Initialize bindings from parent classes
	if (IsValid(UserWidget))
	{
		const UWidgetBlueprintGeneratedClass* BPClass = Cast<UWidgetBlueprintGeneratedClass>(WidgetClass->GetSuperClass());
		while (BPClass != nullptr)
		{
			BPClass->ForEachExtension([this, SpawnedExtension](UWidgetBlueprintGeneratedClassExtension* Extension)
			{
				if (const UMDFastBindingWidgetClassExtension* SuperClassExtension = Cast<UMDFastBindingWidgetClassExtension>(Extension))
				{
					SpawnedExtension->AddSuperBindingContainer(SuperClassExtension->GetBindingContainer());
				}
			});

			BPClass = Cast<UWidgetBlueprintGeneratedClass>(BPClass->GetSuperClass());
		}
	}
}

bool UMDFastBindingWidgetClassExtension::HasBindings() const
{
	return BindingContainer != nullptr && BindingContainer->HasBindings();
}

#if WITH_EDITOR
void UMDFastBindingWidgetClassExtension::SetBindingContainer(UMDFastBindingContainer* BPBindingContainer)
{
	BindingContainer = DuplicateObject(BPBindingContainer, this);
}
#endif
