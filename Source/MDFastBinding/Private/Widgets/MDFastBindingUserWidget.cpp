#include "Widgets/MDFastBindingUserWidget.h"
#include "MDFastBindingContainer.h"

#if WITH_EDITOR
#include "Editor/WidgetCompilerLog.h"
#endif

void UMDFastBindingUserWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (Bindings == nullptr)
	{
		// Widget instances tend to not get changes to "Instance" UObject properties, so we copy them from the CDO directly
		if (UMDFastBindingUserWidget* CDO = GetClass()->GetDefaultObject<UMDFastBindingUserWidget>())
		{
			if (CDO->Bindings != nullptr)
			{
				Bindings = DuplicateObject<UMDFastBindingContainer>(CDO->Bindings, this);
			}
		}
	}

	if (Bindings != nullptr)
	{
		Bindings->InitializeBindings(this);
	}
}

void UMDFastBindingUserWidget::NativeDestruct()
{
	Super::NativeDestruct();
	
	if (Bindings != nullptr)
	{
		Bindings->TerminateBindings(this);
	}
}

void UMDFastBindingUserWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (Bindings != nullptr)
	{
		Bindings->UpdateBindings(this);
	}
}
