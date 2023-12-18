#include "Customizations/MDFastBindingPropertyBindingExtension.h"

#include "BindingDestinations/MDFastBindingDestination_Function.h"
#include "BindingDestinations/MDFastBindingDestination_Property.h"
#include "BindingValues/MDFastBindingValue_Property.h"
#include "BlueprintExtension/MDFastBindingWidgetBlueprintExtension.h"
#include "Components/Widget.h"
#include "EdGraphSchema_K2.h"
#include "Editor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "MDFastBindingContainer.h"
#include "MDFastBindingEditorModule.h"
#include "MDFastBindingEditorStyle.h"
#include "MDFastBindingInstance.h"
#include "Misc/DataValidation.h"
#include "Misc/MessageDialog.h"
#include "ScopedTransaction.h"
#include "SMDFastBindingEditorWidget.h"
#include "StatusBarSubsystem.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintEditor.h"

namespace MDFastBindingPropertyBinding
{
	static UMDFastBindingContainer* RequestBindingContainer(UWidgetBlueprint* WidgetBlueprint)
	{
		auto* FastBindingExtension = UMDFastBindingWidgetBlueprintExtension::RequestExtension<UMDFastBindingWidgetBlueprintExtension>(WidgetBlueprint);

		UMDFastBindingContainer* BindingContainer = FastBindingExtension->GetBindingContainer();
		if (!IsValid(BindingContainer))
		{
			BindingContainer = NewObject<UMDFastBindingContainer>(FastBindingExtension, NAME_None, RF_Transactional | RF_Public);
			FastBindingExtension->SetBindingContainer(BindingContainer);
		}

		return BindingContainer;
	}

	static UMDFastBindingContainer* GetBindingContainer(const UWidgetBlueprint* WidgetBlueprint)
	{
		const auto* FastBindingExtension = UMDFastBindingWidgetBlueprintExtension::GetExtension<UMDFastBindingWidgetBlueprintExtension>(WidgetBlueprint);
		if (IsValid(FastBindingExtension))
		{
			return FastBindingExtension->GetBindingContainer();
		}

		return nullptr;
	}

	static void NotifyBindingsChanged(UWidgetBlueprint* WidgetBlueprint)
	{
		FBlueprintEditorUtils::MarkBlueprintAsModified(WidgetBlueprint);
		const auto* FastBindingExtension = UMDFastBindingWidgetBlueprintExtension::GetExtension<UMDFastBindingWidgetBlueprintExtension>(WidgetBlueprint);
		if (IsValid(FastBindingExtension))
		{
			FastBindingExtension->OnBindingsUpdatedExternally.Broadcast();
		}
	}

	static TArray<FFieldVariant> BuildFieldPathToProperty(const UWidget* Widget, const FProperty* Property)
	{
		TArray<FFieldVariant> Result;

		bool bIsValid = false;
		for (FFieldVariant Iter(Property); Iter.IsValid(); Iter = Iter.GetOwnerVariant())
		{
			if (Widget->IsA(Iter.Get<UClass>()))
			{
				bIsValid = true;
				break;
			}

			Result.Add(Iter);
		}

		if (!bIsValid)
		{
			return {};
		}

		Algo::Reverse(Result);
		return Result;
	}

	static UMDFastBindingInstance* FindBindingInstance(const UWidgetBlueprint* WidgetBlueprint, const UWidget* Widget, const FProperty* Property)
	{
		UMDFastBindingContainer* BindingContainer = GetBindingContainer(WidgetBlueprint);
		if (!IsValid(BindingContainer))
		{
			return nullptr;
		}

		const FString& SetterFunctionName = Property->GetMetaData(FBlueprintMetadata::MD_PropertySetFunction);
		const UFunction* SetterFunction = !SetterFunctionName.IsEmpty() ? Widget->GetClass()->FindFunctionByName(*SetterFunctionName) : nullptr;
		const TArray<FFieldVariant> FieldPath = BuildFieldPathToProperty(Widget, Property);
		const TArray<FFieldVariant> WidgetFieldPath = { WidgetBlueprint->GeneratedClass->FindPropertyByName(Widget->GetFName()) };

		const TArray<UMDFastBindingInstance*>& Bindings = BindingContainer->GetBindings();
		for (UMDFastBindingInstance* Binding : Bindings)
		{
			if (IsValid(Binding))
			{
				UMDFastBindingValue_Property* WidgetNode = nullptr;
				UMDFastBindingDestinationBase* DestinationNode = Binding->GetBindingDestination();
				if (auto* FunctionNode = Cast<UMDFastBindingDestination_Function>(DestinationNode))
				{
					if (FunctionNode->GetFunction() == SetterFunction)
					{
						WidgetNode = Cast<UMDFastBindingValue_Property>(FunctionNode->FindBindingItemValue(TEXT("Function Owner")));
					}
				}
				else if (auto* PropertyNode = Cast<UMDFastBindingDestination_Property>(DestinationNode))
				{
					if (PropertyNode->GetFieldPath() == FieldPath)
					{
						WidgetNode = Cast<UMDFastBindingValue_Property>(PropertyNode->FindBindingItemValue(TEXT("Path Root")));
					}
				}

				if (IsValid(WidgetNode) && WidgetNode->GetFieldPath() == WidgetFieldPath)
				{
					return Binding;
				}
			}
		}

		return nullptr;
	}

	static void OpenBinding(UWidgetBlueprint* WidgetBlueprint, UMDFastBindingInstance* Binding)
	{
		FMDFastBindingEditorModule::OpenBindingEditor(WidgetBlueprint);
		
		if (IAssetEditorInstance* AssetEditorInstance = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(WidgetBlueprint, false))
		{
			const TSharedPtr<FTabManager> TabManager = AssetEditorInstance->GetAssociatedTabManager();
			if (TabManager.IsValid() && TabManager->HasTabSpawner(FMDFastBindingEditorSummoner::TabId))
			{
				if (const TSharedPtr<SDockTab> Tab = TabManager->FindExistingLiveTab(FMDFastBindingEditorSummoner::TabId))
				{
					const TSharedPtr<SMDFastBindingEditorWidget> BindingEditorWidget = StaticCastSharedRef<SMDFastBindingEditorWidget>(Tab->GetContent());
					BindingEditorWidget->SelectBinding(Binding);
				}
			}
		}
	}

	static void CreateBinding(const UWidgetBlueprint* WidgetBlueprint, const UWidget* Widget, const FProperty* Property)
	{
		UMDFastBindingContainer* Container = RequestBindingContainer(const_cast<UWidgetBlueprint*>(WidgetBlueprint));

		FScopedTransaction Transaction = FScopedTransaction(FText::Format(INVTEXT("Created Binding for {0}"), Property->GetDisplayNameText()));
		Container->Modify();
		UMDFastBindingInstance* Binding = Container->AddBinding();
		Binding->BindingName = FString::Printf(TEXT("Set %s %s"), *GetNameSafe(Widget), *(Property->GetDisplayNameText().ToString()));

		UMDFastBindingDestinationBase* DestinationNode = nullptr;
		FName WidgetPinName = NAME_None;

		const FString& SetterFunctionName = Property->GetMetaData(FBlueprintMetadata::MD_PropertySetFunction);
		if (!SetterFunctionName.IsEmpty())
		{
			UFunction* SetterFunction = Widget->GetClass()->FindFunctionByName(*SetterFunctionName);
			check(SetterFunction);

			auto* FunctionDestination = Cast<UMDFastBindingDestination_Function>(Binding->SetDestination(UMDFastBindingDestination_Function::StaticClass()));
			FunctionDestination->SetFunction(SetterFunction);
			DestinationNode = FunctionDestination;
			WidgetPinName = TEXT("Function Owner");
		}
		else
		{
			auto* PropertyDestination = Cast<UMDFastBindingDestination_Property>(Binding->SetDestination(UMDFastBindingDestination_Property::StaticClass()));
			PropertyDestination->SetFieldPath(BuildFieldPathToProperty(Widget, Property));
			DestinationNode = PropertyDestination;
			WidgetPinName = TEXT("Path Root");
		}

		DestinationNode->BindingObjectIdentifier = FGuid::NewGuid();
		DestinationNode->NodePos = FIntPoint(400, 0);
		DestinationNode->SetupBindingItems_Internal();

		UMDFastBindingValue_Property* WidgetNode = Cast<UMDFastBindingValue_Property>(DestinationNode->SetBindingItem(WidgetPinName, UMDFastBindingValue_Property::StaticClass()));
		WidgetNode->SetUpdateType(EMDFastBindingUpdateType::Once);
		WidgetNode->SetFieldPath({ WidgetBlueprint->GeneratedClass->FindPropertyByName(Widget->GetFName()) });
		WidgetNode->BindingObjectIdentifier = FGuid::NewGuid();

		NotifyBindingsChanged(const_cast<UWidgetBlueprint*>(WidgetBlueprint));
		OpenBinding(const_cast<UWidgetBlueprint*>(WidgetBlueprint), Binding);
	}

	static void DeleteBinding(const UWidgetBlueprint* WidgetBlueprint, const UWidget* Widget, const FProperty* Property)
	{
		const EAppReturnType::Type ReturnType = FMessageDialog::Open(EAppMsgType::OkCancel, INVTEXT("Are you sure you want to delete this binding?"));
		if (ReturnType == EAppReturnType::Ok)
		{
			UMDFastBindingContainer* BindingContainer = GetBindingContainer(WidgetBlueprint);
			if (IsValid(BindingContainer))
			{
				UMDFastBindingInstance* BindingInstance = FindBindingInstance(WidgetBlueprint, Widget, Property);
				if (IsValid(BindingInstance))
				{
					FScopedTransaction Transaction = FScopedTransaction(FText::Format(INVTEXT("Deleted Binding ({0}) for {1}"), FText::FromString(BindingInstance->BindingName), Property->GetDisplayNameText()));
					BindingContainer->Modify();

					BindingContainer->DeleteBinding(BindingInstance);

					NotifyBindingsChanged(const_cast<UWidgetBlueprint*>(WidgetBlueprint));
				}
			}
		}
	}

	static void AddFastBindingMenuItems(FMenuBuilder& MenuBuilder, const UWidgetBlueprint* WidgetBlueprint, const UWidget* Widget, const FProperty* Property)
	{
		MenuBuilder.BeginSection("MDFastBinding", INVTEXT("Fast Bindings"));

		if (UMDFastBindingInstance* Binding = FindBindingInstance(WidgetBlueprint, Widget, Property))
		{
			MenuBuilder.AddMenuEntry(
				INVTEXT("Show in Binding Editor"),
				FText::Format(INVTEXT("Open the Binding Editor to the binding that binds to {0}."), Property->GetDisplayNameText()),
				FSlateIcon(FMDFastBindingEditorStyle::GetStyleSetName(), "Icon.FastBinding_16x"),
				FUIAction(FExecuteAction::CreateStatic(&OpenBinding, const_cast<UWidgetBlueprint*>(WidgetBlueprint), Binding))
			);

			MenuBuilder.AddMenuEntry(
				INVTEXT("Delete Fast Binding"),
				FText::Format(INVTEXT("Deletes the binding that binds to {0}."), Property->GetDisplayNameText()),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "Cross"),
				FUIAction(FExecuteAction::CreateStatic(&DeleteBinding, WidgetBlueprint, Widget, Property))
			);
		}
		else
		{
			MenuBuilder.AddMenuEntry(
				INVTEXT("Create Fast Binding"),
				FText::Format(INVTEXT("Creates a binding in the Binding Editor that binds to {0}."), Property->GetDisplayNameText()),
				FSlateIcon(FAppStyle::GetAppStyleSetName(), "Plus"),
				FUIAction(FExecuteAction::CreateStatic(&CreateBinding, WidgetBlueprint, Widget, Property))
			);
		}


		MenuBuilder.EndSection();
	}
}


bool FMDFastBindingPropertyBindingExtension::CanExtend(const UWidgetBlueprint* WidgetBlueprint, const UWidget* Widget, const FProperty* Property) const
{
	if (!IsValid(WidgetBlueprint) || !IsValid(Widget) || Property == nullptr)
	{
		return false;
	}

	// Widget must be a variable
	if (!Widget->bIsVariable || WidgetBlueprint->GeneratedClass->FindPropertyByName(Widget->GetFName()) == nullptr)
	{
		return false;
	}

	// Property must be BlueprintReadWrite and/or have a BlueprintSetter function
	const FString& SetterFunctionName = Property->GetMetaData(FBlueprintMetadata::MD_PropertySetFunction);
	if (!SetterFunctionName.IsEmpty())
	{
		return true;
	}

	if (!Property->HasAnyPropertyFlags(CPF_BlueprintVisible) || Property->HasAnyPropertyFlags(CPF_BlueprintReadOnly))
	{
		return false;
	}

	return !MDFastBindingPropertyBinding::BuildFieldPathToProperty(Widget, Property).IsEmpty();
}

TSharedPtr<FExtender> FMDFastBindingPropertyBindingExtension::CreateMenuExtender(const UWidgetBlueprint* WidgetBlueprint, const UWidget* Widget, const FProperty* Property)
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 3
{
	return nullptr;
}

TSharedPtr<FExtender> FMDFastBindingPropertyBindingExtension::CreateMenuExtender(const UWidgetBlueprint* WidgetBlueprint, UWidget* Widget, TSharedPtr<IPropertyHandle> WidgetPropertyHandle)
#endif
{
	TSharedPtr<FExtender> Extender = MakeShared<FExtender>();
	Extender->AddMenuExtension("BindingActions", EExtensionHook::Before, nullptr, FMenuExtensionDelegate::CreateStatic(&MDFastBindingPropertyBinding::AddFastBindingMenuItems, WidgetBlueprint,
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 3
		(const UWidget*)Widget,
		(const FProperty*)(WidgetPropertyHandle.IsValid() ? WidgetPropertyHandle->GetProperty() : nullptr)
#else
		Widget,
		Property
#endif
	));
	return Extender;
}

#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 3
IPropertyBindingExtension::EDropResult FMDFastBindingPropertyBindingExtension::OnDrop(const FGeometry& Geometry, const FDragDropEvent& DragDropEvent, UWidgetBlueprint* WidgetBlueprint, UWidget* Widget, TSharedPtr<IPropertyHandle> WidgetPropertyHandle)
{
	return IPropertyBindingExtension::EDropResult::Unhandled;
}
#endif

void FMDFastBindingPropertyBindingExtension::ClearCurrentValue(const UWidgetBlueprint* WidgetBlueprint, const UWidget* Widget, const FProperty* Property)
{
	MDFastBindingPropertyBinding::DeleteBinding(WidgetBlueprint, Widget, Property);
}

TOptional<FName> FMDFastBindingPropertyBindingExtension::GetCurrentValue(const UWidgetBlueprint* WidgetBlueprint, const UWidget* Widget, const FProperty* Property) const
{
	if (const UMDFastBindingInstance* BindingInstance = MDFastBindingPropertyBinding::FindBindingInstance(WidgetBlueprint, Widget, Property))
	{
		return FName(BindingInstance->BindingName);
	}

	return {};
}

#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 2
const FSlateBrush* FMDFastBindingPropertyBindingExtension::GetCurrentIcon(const UWidgetBlueprint* WidgetBlueprint, const UWidget* Widget, const FProperty* Property) const
{
	if (UMDFastBindingInstance* BindingInstance = MDFastBindingPropertyBinding::FindBindingInstance(WidgetBlueprint, Widget, Property))
	{
		FDataValidationContext ValidationContext;
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 3
		if (((const UObject*)BindingInstance)->IsDataValid(ValidationContext) == EDataValidationResult::Invalid)
#else
		if (((UObject*)BindingInstance)->IsDataValid(ValidationContext) == EDataValidationResult::Invalid)
#endif
		{
			return FAppStyle::Get().GetBrush("Icons.ErrorWithColor");
		}

		if (BindingInstance->IsBindingPerformant())
		{
			return FMDFastBindingEditorStyle::Get().GetBrush("Icon.Flame");
		}
		else
		{
			return FMDFastBindingEditorStyle::Get().GetBrush("Icon.Clock");
		}
	}

	return nullptr;
}
#endif
