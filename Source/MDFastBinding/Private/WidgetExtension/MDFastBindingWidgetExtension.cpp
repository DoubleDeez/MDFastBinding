#include "WidgetExtension/MDFastBindingWidgetExtension.h"

#include "MDFastBindingContainer.h"
#include "Blueprint/UserWidget.h"
#include "Widgets/IToolTip.h"

void UMDFastBindingWidgetExtension::Construct()
{
	Super::Construct();

	TickingContainers.Insert(false, 0, SuperBindingContainers.Num() + 1);

	if (UUserWidget* UserWidget = GetUserWidget())
	{
		if (BindingContainer != nullptr)
		{
			BindingContainer->InitializeBindings(UserWidget);
			TickingContainers[0] = BindingContainer->DoesNeedTick();
		}

		for (int32 i = 0; i < SuperBindingContainers.Num(); ++i)
		{
			if (UMDFastBindingContainer* SuperBindingContainer = SuperBindingContainers[i])
			{
				SuperBindingContainer->InitializeBindings(UserWidget);
				TickingContainers[i + 1] = SuperBindingContainer->DoesNeedTick();
			}
		}
	}
}

void UMDFastBindingWidgetExtension::Destruct()
{
	Super::Destruct();

	TickingContainers.Reset();

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

	// Tick is called on this if the widget ticks for any reason (even if we don't want to)
	// So only update tick if we actual did any updating because we wanted to
	if (RequiresTick())
	{
		UpdateBindings();

		// If we no longer need to tick (but we did previously), then maybe our widget doesn't need to tick at all
		if (!RequiresTick())
		{
			if (UUserWidget* UserWidget = GetUserWidget())
			{
				UserWidget->UpdateCanTick();
			}
		}
	}
}

bool UMDFastBindingWidgetExtension::RequiresTick() const
{
	return TickingContainers.Contains(true);
}

void UMDFastBindingWidgetExtension::UpdateBindings()
{
	if (UUserWidget* UserWidget = GetUserWidget())
	{
		for (TConstSetBitIterator<> It(TickingContainers); It; ++It)
		{
			const int32 Index = It.GetIndex();
			UMDFastBindingContainer* Container = (Index == 0) ? BindingContainer : SuperBindingContainers[Index - 1];
			if (Container != nullptr && Container->DoesNeedTick())
			{
				Container->UpdateBindings(UserWidget);
				TickingContainers[Index] = Container->DoesNeedTick();
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

void UMDFastBindingWidgetExtension::UpdateNeedsTick()
{
	const bool bDidNeedTick = RequiresTick();

	if (BindingContainer != nullptr)
	{
		TickingContainers[0] = BindingContainer->DoesNeedTick();
	}

	for (int32 i = 0; i < SuperBindingContainers.Num(); ++i)
	{
		if (const UMDFastBindingContainer* SuperBindingContainer = SuperBindingContainers[i])
		{
			TickingContainers[i + 1] = SuperBindingContainer->DoesNeedTick();
		}
	}

	if (bDidNeedTick != RequiresTick())
	{
		if (UUserWidget* UserWidget = GetUserWidget())
		{
			UserWidget->UpdateCanTick();
		}
	}
}
