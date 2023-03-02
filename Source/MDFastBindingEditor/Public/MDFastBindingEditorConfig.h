#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MDFastBindingEditorConfig.generated.h"

UCLASS(config = "FastBindingEditor", meta = (DisplayName = "Fast Binding Editor"))
class MDFASTBINDINGEDITOR_API UMDFastBindingEditorConfig : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UMDFastBindingEditorConfig();
	
	virtual FName GetContainerName() const override { return TEXT("Editor"); }
	
	bool ShouldRunBindingsAtDesignTime() const { return bShouldRunBindingsAtDesignTime; }
	void ToggleShouldRunBindingsAtDesignTime();

	mutable FSimpleMulticastDelegate OnShouldRunBindingsAtDesignTimeChanged;
	
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

protected:
	// If true, bindings on User Widgets will execute during design time.
	// This is useful for previewing bindings that are intended to replace PreConstruct script
	// or for bindings that interact with Widget Animations
	UPROPERTY(EditDefaultsOnly, Config, Category = "UMG")
	bool bShouldRunBindingsAtDesignTime = true;
};
