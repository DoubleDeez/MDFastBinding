#include "MDFastBindingEditorConfig.h"

UMDFastBindingEditorConfig::UMDFastBindingEditorConfig()
{
	CategoryName = TEXT("Plugins");
	SectionName = TEXT("Fast Binding Editor");
}

void UMDFastBindingEditorConfig::ToggleShouldRunBindingsAtDesignTime()
{
	bShouldRunBindingsAtDesignTime = !bShouldRunBindingsAtDesignTime;
	SaveConfig();
	OnShouldRunBindingsAtDesignTimeChanged.Broadcast();
}

#if WITH_EDITOR
void UMDFastBindingEditorConfig::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UMDFastBindingEditorConfig, bShouldRunBindingsAtDesignTime))
	{
		OnShouldRunBindingsAtDesignTimeChanged.Broadcast();
	}
}
#endif
