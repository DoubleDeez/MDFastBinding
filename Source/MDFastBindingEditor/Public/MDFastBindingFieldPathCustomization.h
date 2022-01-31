
#pragma once

#include "CoreMinimal.h"
#include "IPropertyTypeCustomization.h"
#include "MDFastBindingFieldPath.h"

class UMDFastBindingContainer;

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

	void BuildFieldPathMenu(FMenuBuilder& MenuBuilder, UStruct* InStruct, TArray<FName> ParentPath) const;

	void SetFieldPath(TArray<FName> Path) const;

	void UpdateComboButton() const;
	
	TSharedPtr<IPropertyHandle> FieldPathHandle;
	TSharedPtr<IPropertyHandleArray> FieldPathNamesHandle;
	TSharedPtr<SBox> ComboButtonContent;
};



