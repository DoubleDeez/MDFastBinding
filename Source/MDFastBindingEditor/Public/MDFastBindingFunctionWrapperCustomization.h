
#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "MDFastBindingFunctionWrapper.h"

class UMDFastBindingContainer;

class FMDFastBindingFunctionWrapperCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance() { return MakeShared<FMDFastBindingFunctionWrapperCustomization>(); }

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override {}

private:
	FText GetComboButtonText();

	FMDFastBindingFunctionWrapper* ResolveFunctionWrapper() const;

	TSharedRef<SWidget> GetPathSelectorContent();

	void GatherPossibleFunctions();

	void UpdateComboButton();

	void OnFunctionSelected(UFunction* Function, ESelectInfo::Type SelectType);
	
	TSharedPtr<IPropertyHandle> FunctionWrapperHandle;
	TSharedPtr<IPropertyHandle> FunctionWrapperNameHandle;
	TSharedPtr<SBox> ComboButtonContent;
	TSharedPtr<SComboButton> ComboButton;

	TArray<UFunction*> Functions;
};



