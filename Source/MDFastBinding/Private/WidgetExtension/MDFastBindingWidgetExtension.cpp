#include "WidgetExtension/MDFastBindingWidgetExtension.h"

#include "MDFastBindingContainer.h"
#include "Blueprint/UserWidget.h"

void UMDFastBindingWidgetExtension::Construct()
{
	Super::Construct();

	if (UUserWidget* UserWidget = GetUserWidget())
	{
		if (BindingContainer != nullptr)
		{
			BindingContainer->InitializeBindings(UserWidget);
		}

		for (UMDFastBindingContainer* SuperBindingContainer : SuperBindingContainers)
		{
			if (SuperBindingContainer != nullptr)
			{
				SuperBindingContainer->InitializeBindings(UserWidget);
			}
		}
	}
}

void UMDFastBindingWidgetExtension::Destruct()
{
	Super::Destruct();

	if (UUserWidget* UserWidget = GetUserWidget())
	{
		if (BindingContainer != nullptr)
		{
			BindingContainer->TerminateBindings(UserWidget);
		}

		for (UMDFastBindingContainer* SuperBindingContainer : SuperBindingContainers)
		{
			if (SuperBindingContainer != nullptr)
			{
				SuperBindingContainer->TerminateBindings(UserWidget);
			}
		}
	}
}

void UMDFastBindingWidgetExtension::Tick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::Tick(MyGeometry, InDeltaTime);

	UpdateBindings();
}

void UMDFastBindingWidgetExtension::UpdateBindings()
{
	if (UUserWidget* UserWidget = GetUserWidget())
	{
		if (BindingContainer != nullptr)
		{
			BindingContainer->UpdateBindings(UserWidget);
		}

		for (UMDFastBindingContainer* SuperBindingContainer : SuperBindingContainers)
		{
			if (SuperBindingContainer != nullptr)
			{
				SuperBindingContainer->UpdateBindings(UserWidget);
			}
		}
	}
}

void UMDFastBindingWidgetExtension::SetBindingContainer(const UMDFastBindingContainer* CDOBindingContainer)
{
	BindingContainer = DuplicateObject(CDOBindingContainer, this);
}

void UMDFastBindingWidgetExtension::AddSuperBindingContainer(const UMDFastBindingContainer* SuperCDOBindingContainer)
{
	if (UMDFastBindingContainer* SuperBindingContainer = DuplicateObject(SuperCDOBindingContainer, this))
	{
		SuperBindingContainers.Add(SuperBindingContainer);
	}
}

UClass* UMDFastBindingWidgetExtension::GetBindingOwnerClass() const
{
	if (const UUserWidget* Widget = GetUserWidget())
	{
		return Widget->GetClass();
	}

	return nullptr;
}
