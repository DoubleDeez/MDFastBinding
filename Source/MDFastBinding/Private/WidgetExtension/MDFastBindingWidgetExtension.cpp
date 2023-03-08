#include "WidgetExtension/MDFastBindingWidgetExtension.h"

#include "MDFastBindingContainer.h"
#include "Blueprint/UserWidget.h"

void UMDFastBindingWidgetExtension::Construct()
{
	Super::Construct();

	if (BindingContainer != nullptr)
	{
		BindingContainer->InitializeBindings(GetUserWidget());
	}
}

void UMDFastBindingWidgetExtension::Destruct()
{
	Super::Destruct();

	if (BindingContainer != nullptr)
	{
		BindingContainer->TerminateBindings(GetUserWidget());
	}
}

void UMDFastBindingWidgetExtension::Tick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::Tick(MyGeometry, InDeltaTime);

	UpdateBindings();
}

void UMDFastBindingWidgetExtension::UpdateBindings()
{
	if (BindingContainer != nullptr)
	{
		BindingContainer->UpdateBindings(GetUserWidget());
	}
}

void UMDFastBindingWidgetExtension::SetBindingContainer(const UMDFastBindingContainer* CDOBindingContainer)
{
	BindingContainer = DuplicateObject(CDOBindingContainer, this);
	BindingContainer->GetBindingOwnerClassDelegate.BindUObject(this, &UMDFastBindingWidgetExtension::GetBindingOwnerClass);
}

UClass* UMDFastBindingWidgetExtension::GetBindingOwnerClass() const
{
	if (const UUserWidget* Widget = GetUserWidget())
	{
		return Widget->GetClass();
	}

	return nullptr;
}
