#include "MDFastBindingFunctionWrapperCustomization.h"

#include "DetailWidgetRow.h"
#include "MDFastBindingHelpers.h"
#include "Kismet2/BlueprintEditorUtils.h"

#define LOCTEXT_NAMESPACE "MDFastBindingFunctionWrapperCustomization"

void FMDFastBindingFunctionWrapperCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle,
                                                           FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	FunctionWrapperHandle = PropertyHandle;
	FunctionWrapperMemberHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FMDFastBindingFunctionWrapper, FunctionMember));
	
	HeaderRow.NameContent()
	[
		PropertyHandle->CreatePropertyNameWidget()
	]
	.ValueContent()
	.HAlign(HAlign_Fill)
	[
		SAssignNew(ComboButton, SComboButton)
		.OnGetMenuContent(this, &FMDFastBindingFunctionWrapperCustomization::GetPathSelectorContent)
		.ContentPadding(FMargin( 2.0f, 2.0f ))
		.ButtonContent()
		[
			SAssignNew(ComboButtonContent, SBox)
		]
	];

	UpdateComboButton();
}

TSharedRef<SWidget> FMDFastBindingFunctionWrapperCustomization::GetComboButtonContent()
{
	if (FMDFastBindingFunctionWrapper* FunctionWrapper = ResolveFunctionWrapper())
	{
		UClass* OwnerClass = FunctionWrapper->GetFunctionOwnerClass();
		if (OwnerClass == nullptr)
		{
			return SNew(STextBlock).Text(LOCTEXT("InvalidFunctionOwner", "The function owner is invalid"));
		}

		GatherPossibleFunctions();
		if (Functions.Num() == 0)
		{
			return SNew(STextBlock).Text(FText::Format(LOCTEXT("NoOptionsForFunction", "{0} has 0 valid functions"), OwnerClass->GetDisplayNameText()));
		}

		FunctionWrapper->BuildFunctionData();
		UFunction* FunctionPtr = FunctionWrapper->GetFunctionPtr();
		if (FunctionPtr == nullptr)
		{
			return SNew(STextBlock).Text(LOCTEXT("NullFunction", "None"));
		}

		return BuildFunctionWidget(FunctionPtr);
	}

	return SNullWidget::NullWidget;
}

FMDFastBindingFunctionWrapper* FMDFastBindingFunctionWrapperCustomization::ResolveFunctionWrapper() const
{
	if (FunctionWrapperHandle.IsValid())
	{
		void* ValuePtr = nullptr;
		if (FunctionWrapperHandle->GetValueData(ValuePtr) == FPropertyAccess::Success)
		{
			return static_cast<FMDFastBindingFunctionWrapper*>(ValuePtr);
		}
	}

	return nullptr;
}

TSharedRef<SWidget> FMDFastBindingFunctionWrapperCustomization::GetPathSelectorContent()
{
	GatherPossibleFunctions();
	
	FMenuBuilder MenuBuilder(true, nullptr);
	for (UFunction* Func : Functions)
	{
		FUIAction OnFunctionSelected = FUIAction(FExecuteAction::CreateSP(this, &FMDFastBindingFunctionWrapperCustomization::OnFunctionSelected, Func));
		MenuBuilder.AddMenuEntry(OnFunctionSelected, BuildFunctionWidget(Func));
	}

	return MenuBuilder.MakeWidget();
}

TSharedRef<SWidget> FMDFastBindingFunctionWrapperCustomization::BuildFunctionWidget(UFunction* Function) const
{
	if (Function == nullptr)
	{
		// Something's wrong, the text might have info
		return SNew(STextBlock)
			.Text(LOCTEXT("NullFunctionName", "None"))
			.ToolTipText(LOCTEXT("NullFunctionTooltip", "Clear the selection"));
	}
	
	const FProperty* ReturnProp = nullptr;
	TArray<const FProperty*> Params;
	FMDFastBindingHelpers::SplitFunctionParamsAndReturnProp(Function, Params, ReturnProp);
	
	const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
	FEdGraphPinType PinType;
	Schema->ConvertPropertyToPinType(ReturnProp, PinType);

	return SNew(SHorizontalBox)
		+SHorizontalBox::Slot()
		.AutoWidth()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		.Padding(1.f, 0.f)
		[
			SNew(SImage)
			.Image(FEditorStyle::Get().GetBrush(TEXT("GraphEditor.Function_16x")))
			.ColorAndOpacity(ReturnProp == nullptr
				? FLinearColor::White
				: Schema->GetPinTypeColor(PinType))
			.ToolTipText(ReturnProp == nullptr
				? FText::FromString(TEXT("void"))
				: FText::FromString(FMDFastBindingHelpers::PropertyToString(*ReturnProp)))
		]
		+SHorizontalBox::Slot()
		.HAlign(HAlign_Left)
		.VAlign(VAlign_Center)
		.Padding(4.f, 0.f)
		[
			SNew(STextBlock)
			.Text(Function->GetDisplayNameText())
			.ToolTipText(FText::Format(INVTEXT("{0}\n{1}"), FText::FromString(FMDFastBindingFunctionWrapper::FunctionToString(Function)), Function->GetToolTipText()))
		];
}

void FMDFastBindingFunctionWrapperCustomization::GatherPossibleFunctions()
{
	Functions.Empty();
	
	if (const FMDFastBindingFunctionWrapper* FunctionWrapper = ResolveFunctionWrapper())
	{
		if (const UClass* Class = FunctionWrapper->GetFunctionOwnerClass())
		{
			for (TFieldIterator<UFunction> It(Class); It; ++It)
			{
				if (FMDFastBindingFunctionWrapper::IsFunctionValidForWrapper(*It))
				{
					if (FunctionWrapper->FunctionFilter.IsBound())
					{
						const FProperty* ReturnProp = nullptr;
						TArray<const FProperty*> Params;
						FMDFastBindingHelpers::SplitFunctionParamsAndReturnProp(*It, Params, ReturnProp);
						if (!FunctionWrapper->FunctionFilter.Execute(*It, ReturnProp, Params))
						{
							continue;
						}
					}
					
					Functions.Add(*It);
				}
			}
		}
	}
	
	Functions.Sort([](const UFunction& A, const UFunction& B)
	{
		return A.GetDisplayNameText().CompareTo(B.GetDisplayNameText()) < 0;
	});

	Functions.Insert(nullptr, 0);
}

void FMDFastBindingFunctionWrapperCustomization::UpdateComboButton()
{
	if (ComboButtonContent.IsValid())
	{
		ComboButtonContent->SetContent(GetComboButtonContent());
	}
}

void FMDFastBindingFunctionWrapperCustomization::OnFunctionSelected(UFunction* Function)
{
	if (FunctionWrapperMemberHandle.IsValid())
	{
		void* ValueData = nullptr;
		if (FunctionWrapperMemberHandle->GetValueData(ValueData) == FPropertyAccess::Success)
		{
			if (FMDFastBindingMemberReference* MemberRef = static_cast<FMDFastBindingMemberReference*>(ValueData))
			{
				if (Function != nullptr)
				{
					MemberRef->SetFromField<UFunction>(Function, false);
				}
				else
				{
					*MemberRef = {};
				}
				
				MemberRef->bIsFunction = true;

				FunctionWrapperHandle->NotifyPostChange(EPropertyChangeType::ValueSet);
			}
		}

		UpdateComboButton();

		if (ComboButton.IsValid())
		{
			ComboButton->SetIsOpen(false);
		}
	}
}

#undef LOCTEXT_NAMESPACE
