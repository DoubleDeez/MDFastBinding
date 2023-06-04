#pragma once

#include "DesignerExtension.h"
#include "Blueprint/UserWidget.h"
#include "UObject/StrongObjectPtr.h"

class UMDFastBindingContainer;
class IDesignerExtensionFactory;

class MDFASTBINDINGEDITOR_API FMDFastBindingDesignerExtension : public FDesignerExtension
{
public:
	static TSharedRef<IDesignerExtensionFactory> MakeFactory();

	FMDFastBindingDesignerExtension();

	virtual void Initialize(IUMGDesigner* InDesigner, UWidgetBlueprint* InBlueprint) override;

	virtual void Uninitialize() override;

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	virtual void PreviewContentChanged(TSharedRef<SWidget> NewContent) override;

private:
	void InitializeBindingInstances();
	void UpdateBindingInstances();
	void TerminateBindingInstances();

	void InitializeBindingInstanceForWidget(UUserWidget* Widget);
	void InitializeBindingContainerForWidget(const UMDFastBindingContainer* CDOBindingContainer, UUserWidget* Widget);

	void OnShouldRunBindingsAtDesignTimeChanged();

	UUserWidget* GetPreviewWidget() const;
	FWidgetBlueprintEditor* FindWidgetEditor() const;

	TMap<TWeakObjectPtr<UUserWidget>, TArray<TStrongObjectPtr<UMDFastBindingContainer>>> BindingContainers;
	TWeakObjectPtr<UUserWidget> PreviewedWidget = nullptr;
};
