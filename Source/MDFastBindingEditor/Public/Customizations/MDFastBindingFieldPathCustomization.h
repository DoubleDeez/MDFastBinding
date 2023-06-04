
#pragma once

#include "IPropertyTypeCustomization.h"
#include "MDFastBindingFieldPath.h"

class FMenuBuilder;
class UMDFastBindingContainer;
class IPropertyHandle;
class IPropertyHandleArray;
class SBox;

class FMDFastBindingFieldPathCustomization : public IPropertyTypeCustomization
{
public:
	static TSharedRef<IPropertyTypeCustomization> MakeInstance() { return MakeShared<FMDFastBindingFieldPathCustomization>(); }

	virtual void CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle, FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils) override;
	virtual void CustomizeChildren(TSharedRef<IPropertyHandle> PropertyHandle, IDetailChildrenBuilder& ChildBuilder, IPropertyTypeCustomizationUtils& CustomizationUtils) override {}

private:
	FText GetComboButtonText() const;

	FMDFastBindingFieldPath* ResolveFieldPath() const;

	TSharedRef<SWidget> GetPathSelectorContent() const;
	TSharedRef<SWidget> BuildPropertyWidget(const FProperty* InProperty, const FText& InText, const FText& InToolTip, bool bIsFunction) const;

	TArray<FFieldVariant> GatherPossibleFields(UStruct* InStruct) const;

	void BuildFieldPathMenu(FMenuBuilder& MenuBuilder, UStruct* InStruct, TArray<FFieldVariant> ParentPath) const;

	void SetFieldPath(TArray<FFieldVariant> Path) const;

	void UpdateComboButton() const;

	TSharedPtr<IPropertyHandle> FieldPathHandle;
	TSharedPtr<IPropertyHandleArray> FieldPathMembersHandle;
	TSharedPtr<SBox> ComboButtonContent;
};



