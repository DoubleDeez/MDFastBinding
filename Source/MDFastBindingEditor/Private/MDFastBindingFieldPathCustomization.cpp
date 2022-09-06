#include "MDFastBindingFieldPathCustomization.h"

#include "DetailWidgetRow.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "MDFastBindingHelpers.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SComboButton.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "MDFastBindingFieldPathCustomization"

void FMDFastBindingFieldPathCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle,
                                                           FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	FieldPathHandle = PropertyHandle;
	FieldPathMembersHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FMDFastBindingFieldPath, FieldPathMembers))->AsArray();
	
	HeaderRow.NameContent()
	[
		PropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.HAlign(HAlign_Fill)
	[
		SNew(SComboButton)
		.OnGetMenuContent(this, &FMDFastBindingFieldPathCustomization::GetPathSelectorContent)
		.ContentPadding(FMargin( 2.0f, 2.0f ))
		.ButtonContent()
		[
			SAssignNew(ComboButtonContent, SBox)
		]
	];

	UpdateComboButton();
}

FText FMDFastBindingFieldPathCustomization::GetComboButtonText() const
{
	if (FMDFastBindingFieldPath* FieldPath = ResolveFieldPath())
	{
		UStruct* OwnerStruct = FieldPath->GetPathOwnerStruct();
		if (OwnerStruct == nullptr)
		{
			return LOCTEXT("InvalidPathOwner", "The path owner is invalid");
		}
		
		if (GatherPossibleFields(OwnerStruct).Num() == 0)
		{
			return FText::Format(LOCTEXT("NoOptionsForPath", "{0} has 0 valid options"), OwnerStruct->GetDisplayNameText());
		}
		
		if (FieldPath->GetLeafProperty() == nullptr)
		{
			return LOCTEXT("EmptyFieldPathPrompt", "Select a property...");
		}

		return FText::FromString(FieldPath->ToString());
	}

	return FText::GetEmpty();
}

FMDFastBindingFieldPath* FMDFastBindingFieldPathCustomization::ResolveFieldPath() const
{
	if (FieldPathHandle.IsValid())
	{
		void* ValuePtr = nullptr;
		if (FieldPathHandle->GetValueData(ValuePtr) == FPropertyAccess::Success)
		{
			return static_cast<FMDFastBindingFieldPath*>(ValuePtr);
		}
	}

	return nullptr;
}

TSharedRef<SWidget> FMDFastBindingFieldPathCustomization::GetPathSelectorContent() const
{
	constexpr bool bIsRecursivelySearchable = false;
	FMenuBuilder MenuBuilder(true, nullptr, TSharedPtr<FExtender>(), false, &FCoreStyle::Get(), true, NAME_None, bIsRecursivelySearchable);

	if (const FMDFastBindingFieldPath* FieldPath = ResolveFieldPath())
	{
		BuildFieldPathMenu(MenuBuilder, FieldPath->GetPathOwnerStruct(), {});
	}

	return MenuBuilder.MakeWidget();
}

TSharedRef<SWidget> FMDFastBindingFieldPathCustomization::BuildPropertyWidget(const FProperty* InProperty, const FText& InText, const FText& InToolTip, bool bIsFunction) const
{
	if (InProperty == nullptr)
	{
		// Something's wrong, the text might have info
		return SNew(STextBlock)
			.Text(InText)
			.ToolTipText(InToolTip);
	}
	
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
	FEdGraphPinType PinType;
	Schema->ConvertPropertyToPinType(InProperty, PinType);

	return SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(1.f, 0.f)
		[
			SNew(SImage)
			.Image(bIsFunction ? FEditorStyle::Get().GetBrush(TEXT("GraphEditor.Function_16x")) : FBlueprintEditorUtils::GetIconFromPin(PinType, true))
			.ColorAndOpacity(Schema->GetPinTypeColor(PinType))
			.ToolTipText(FText::FromString(FMDFastBindingHelpers::PropertyToString(*InProperty)))
		]
		+SHorizontalBox::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(4.f, 0.f)
		[
			SNew(STextBlock)
			.Text(InText)
			.ToolTipText(InToolTip)
		];
}

TArray<FFieldVariant> FMDFastBindingFieldPathCustomization::GatherPossibleFields(UStruct* InStruct) const
{
	TArray<FFieldVariant> Fields;
	
	if (const FMDFastBindingFieldPath* FieldPath = ResolveFieldPath())
	{
		if (const UClass* Class = Cast<UClass>(InStruct))
		{
			for (TFieldIterator<UFunction> It(Class); It; ++It)
			{
				if (FMDFastBindingFieldPath::IsFunctionValidForPath(**It))
				{
					Fields.Add(*It);
				}
			}
		}

		for (TFieldIterator<const FProperty> It(InStruct); It; ++It)
		{
			if (FieldPath->IsPropertyValidForPath(**It))
			{
				Fields.Add(*It);
			}
		}
	}

	// Functions first, then lexicographic
	Fields.Sort([](const FFieldVariant& A, const FFieldVariant& B)
	{
		if (A.IsUObject() && !B.IsUObject())
		{
			return true;
		}

		if (!A.IsUObject() && B.IsUObject())
		{
			return false;
		}

		return A.GetFName().Compare(B.GetFName()) < 0;
	});
	
	return Fields;
}

void FMDFastBindingFieldPathCustomization::BuildFieldPathMenu(FMenuBuilder& MenuBuilder, UStruct* InStruct, TArray<FFieldVariant> ParentPath) const
{
	TArray<FFieldVariant> Fields = GatherPossibleFields(InStruct);
	for (const FFieldVariant& Field : Fields)
	{		
		TArray<FFieldVariant> Path = ParentPath;
		const FProperty* FieldProp = nullptr;
		FText DisplayName;
		FText ToolTip;
		bool bIsFunction = false;
		if (const UFunction* Func = Cast<UFunction>(Field.ToUObject()))
		{
			static const FText FuncToolTipFormat = LOCTEXT("FunctionMenuItemToolTipFormat", "{0} (returns {1}) \n{2}");
			TArray<const FProperty*> Params;
			FMDFastBindingHelpers::SplitFunctionParamsAndReturnProp(Func, Params, FieldProp);
			
			Path.Add(Func);
			DisplayName = Func->GetDisplayNameText();
			ToolTip = FText::Format(FuncToolTipFormat, FText::FromName(Func->GetFName()), FText::FromString(FMDFastBindingHelpers::PropertyToString(*FieldProp)), Func->GetToolTipText());
			bIsFunction = true;
		}
		else
		{
			static const FText PropToolTipFormat = LOCTEXT("PropertyMenuItemToolTipFormat", "{0} ({1}) \n{2}");
			
			FieldProp = CastField<const FProperty>(Field.ToField());
			Path.Add(FieldProp);
			DisplayName = FText::FromName(FieldProp->GetFName());
			ToolTip = FText::Format(PropToolTipFormat, FieldProp->GetDisplayNameText(), FText::FromString(FMDFastBindingHelpers::PropertyToString(*FieldProp)), FieldProp->GetToolTipText());
		}

		FUIAction OnFieldSelected = FUIAction(FExecuteAction::CreateSP(this, &FMDFastBindingFieldPathCustomization::SetFieldPath, Path));
		const FObjectPropertyBase* ObjectProp = CastField<const FObjectPropertyBase>(FieldProp);
		const FStructProperty* StructProp = CastField<const FStructProperty>(FieldProp);
		if (ObjectProp != nullptr && GatherPossibleFields(ObjectProp->PropertyClass).Num() > 0)
		{
			FNewMenuDelegate NewMenuDelegate = FNewMenuDelegate::CreateSP(this, &FMDFastBindingFieldPathCustomization::BuildFieldPathMenu, static_cast<UStruct*>(ObjectProp->PropertyClass), Path);
			MenuBuilder.AddSubMenu(OnFieldSelected, BuildPropertyWidget(FieldProp, DisplayName, ToolTip, bIsFunction), NewMenuDelegate);
		}
		else if (StructProp != nullptr && GatherPossibleFields(StructProp->Struct).Num() > 0)
		{
			FNewMenuDelegate NewMenuDelegate = FNewMenuDelegate::CreateSP(this, &FMDFastBindingFieldPathCustomization::BuildFieldPathMenu, static_cast<UStruct*>(StructProp->Struct), Path);
			MenuBuilder.AddSubMenu(OnFieldSelected, BuildPropertyWidget(FieldProp, DisplayName, ToolTip, bIsFunction), NewMenuDelegate);
		}
		else
		{
			MenuBuilder.AddMenuEntry(OnFieldSelected, BuildPropertyWidget(FieldProp, DisplayName, ToolTip, bIsFunction));
		}

	}
}

void FMDFastBindingFieldPathCustomization::SetFieldPath(TArray<FFieldVariant> Path) const
{
	if (FieldPathMembersHandle.IsValid())
	{
		if (FieldPathHandle.IsValid())
		{
			FieldPathHandle->NotifyPreChange();
		}
		
		FieldPathMembersHandle->EmptyArray();
		for (const FFieldVariant& PathElement : Path)
		{
			FieldPathMembersHandle->AddItem();
			uint32 NewIdx = 0;
			FieldPathMembersHandle->GetNumElements(NewIdx);
			void* ValueData = nullptr;
			if (FieldPathMembersHandle->GetElement(NewIdx - 1)->GetValueData(ValueData) == FPropertyAccess::Success)
			{
				if (FMDFastBindingMemberReference* MemberRef = static_cast<FMDFastBindingMemberReference*>(ValueData))
				{
					if (PathElement.IsA<UFunction>())
					{
						MemberRef->SetFromField<UFunction>(PathElement.Get<UFunction>(), false);
						MemberRef->bIsFunction = true;
					}
					else
					{
						MemberRef->SetFromField<FProperty>(PathElement.Get<FProperty>(), false);
						MemberRef->bIsFunction = false;
					}
				}
			}
		}
		
		UpdateComboButton();

		if (FieldPathHandle.IsValid())
		{
			FieldPathHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
			FieldPathHandle->NotifyFinishedChangingProperties();
		}
	}
}

void FMDFastBindingFieldPathCustomization::UpdateComboButton() const
{
	if (FMDFastBindingFieldPath* FieldPath = ResolveFieldPath())
	{
		const FProperty* LeafProp = FieldPath->GetLeafProperty();
		const FText ComboText = GetComboButtonText();
		const bool bIsLeafFunction = FieldPath->IsLeafFunction();
	
		if (ComboButtonContent.IsValid())
		{
			ComboButtonContent->SetContent(BuildPropertyWidget(LeafProp, ComboText, ComboText, bIsLeafFunction));
		}
	}
}

#undef LOCTEXT_NAMESPACE
