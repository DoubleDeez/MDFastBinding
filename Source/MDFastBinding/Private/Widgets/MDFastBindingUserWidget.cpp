#include "Widgets/MDFastBindingUserWidget.h"
#include "MDFastBindingContainer.h"

#if WITH_EDITOR
#include "Editor/WidgetCompilerLog.h"
#endif

void UMDFastBindingUserWidget::NativeConstruct()
{
	Super::NativeConstruct();

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
