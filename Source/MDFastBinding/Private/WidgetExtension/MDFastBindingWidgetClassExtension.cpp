#include "WidgetExtension/MDFastBindingWidgetClassExtension.h"

#include "Blueprint/UserWidget.h"
#include "MDFastBindingContainer.h"
#include "WidgetExtension/MDFastBindingWidgetExtension.h"

void UMDFastBindingWidgetClassExtension::Initialize(UUserWidget* UserWidget)
{
	Super::Initialize(UserWidget);

	if (BindingContainer != nullptr)
	{
		UserWidget->AddExtension<UMDFastBindingWidgetExtension>()->SetBindingContainer(BindingContainer);
	}
}

#if WITH_EDITOR
void UMDFastBindingWidgetClassExtension::SetBindingContainer(UMDFastBindingContainer* BPBindingContainer)
{
	BindingContainer = DuplicateObject(BPBindingContainer, this);
}
#endif
