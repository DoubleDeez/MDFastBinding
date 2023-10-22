﻿
#pragma once

#include "IPropertyTypeCustomization.h"
#include "MDFastBindingFunctionWrapper.h"

class UMDFastBindingContainer;
class SBox;
class SComboButton;

class FMDFastBindingFunctionWrapperCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance() { return MakeShared<FMDFastBindingFunctionWrapperCustomization>(); }

	static bool IsFunctionValidForWrapper(const UFunction* Func);

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override {}

private:
	TSharedRef<SWidget> GetComboButtonContent();

	FMDFastBindingFunctionWrapper* ResolveFunctionWrapper() const;

	TSharedRef<SWidget> GetPathSelectorContent();

	TSharedRef<SWidget> BuildFunctionWidget(UFunction* Function) const;

	void GatherPossibleFunctions();

	void UpdateComboButton();

	void OnFunctionSelected(UFunction* Function);

	TSharedPtr<IPropertyHandle> FunctionWrapperHandle;
	TSharedPtr<IPropertyHandle> FunctionWrapperMemberHandle;
	TSharedPtr<SBox> ComboButtonContent;
	TSharedPtr<SComboButton> ComboButton;

	TArray<UFunction*> Functions;
};



