#include "MDFastBindingFunctionWrapperCustomization.h"

#include "DetailWidgetRow.h"
#include "MDFastBindingHelpers.h"
#include "Kismet2/BlueprintEditorUtils.h"

#define LOCTEXT_NAMESPACE "MDFastBindingFunctionWrapperCustomization"

void FMDFastBindingFunctionWrapperCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> PropertyHandle,
                                                           FDetailWidgetRow& HeaderRow, IPropertyTypeCustomizationUtils& CustomizationUtils)
{
	FunctionWrapperHandle = PropertyHandle;
	FunctionWrapperNameHandle = PropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FMDFastBindingFunctionWrapper, FunctionName));
	
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

FText FMDFastBindingFunctionWrapperCustomization::GetComboButtonText()
{
	if (FMDFastBindingFunctionWrapper* FunctionWrapper = ResolveFunctionWrapper())
	{
		UClass* OwnerClass = FunctionWrapper->GetFunctionOwnerClass();
		if (OwnerClass == nullptr)
		{
			return LOCTEXT("InvalidFunctionOwner", "The function owner is invalid");
		}

		GatherPossibleFunctions();
		if (Functions.Num() == 0)
		{
			return FText::Format(LOCTEXT("NoOptionsForFunction", "{0} has 0 valid functions"), OwnerClass->GetDisplayNameText());
		}

		return FText::FromString(FunctionWrapper->ToString());
	}

	return FText::GetEmpty();
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
	auto HandleGenerateRow = [this](UFunction* Function, const TSharedRef<STableViewBase>& InOwnerTableView) -> TSharedRef<ITableRow>
	{
		return SNew(STableRow<UFunction*>, InOwnerTableView)
			.Content()
			[
				SNew(STextBlock).Text(FText::FromString(FMDFastBindingFunctionWrapper::FunctionToString(Function)))
			];
	};
	
	return SNew(SBorder)
	[
		SNew(SListView<UFunction*>)
		.ListItemsSource(&Functions)
		.SelectionMode(ESelectionMode::Single)
		.OnGenerateRow_Lambda(HandleGenerateRow)
		.OnSelectionChanged(this, &FMDFastBindingFunctionWrapperCustomization::OnFunctionSelected)
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
		ComboButtonContent->SetContent(SNew(STextBlock).Text(GetComboButtonText()));
	}
}

void FMDFastBindingFunctionWrapperCustomization::OnFunctionSelected(UFunction* Function, ESelectInfo::Type SelectType)
{
	if (FunctionWrapperNameHandle.IsValid())
	{
		if (Function != nullptr)
		{
			FunctionWrapperNameHandle->SetValue(Function->GetFName());
		}
		else
		{
			FunctionWrapperNameHandle->SetValue(NAME_None);
		}

		UpdateComboButton();

		if (ComboButton.IsValid())
		{
			ComboButton->SetIsOpen(false);
		}
	}
}

#undef LOCTEXT_NAMESPACE
