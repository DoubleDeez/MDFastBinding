#include "MDFastBindingEditorModule.h"

#include "BlueprintEditorModule.h"
#include "BlueprintEditorTabs.h"
#include "Customizations/MDFastBindingFieldPathCustomization.h"
#include "Customizations/MDFastBindingFunctionWrapperCustomization.h"
#include "Customizations/MDFastBindingObjectCustomization.h"
#include "Customizations/MDFastBindingPropertyBindingExtension.h"
#include "LevelEditor.h"
#include "MDFastBindingContainer.h"
#include "MDFastBindingDesignerExtension.h"
#include "Util/MDFastBindingEditorConfig.h"
#include "MDFastBindingEditorStyle.h"
#include "MDFastBindingFieldPath.h"
#include "MDFastBindingFunctionWrapper.h"
#include "MDFastBindingInstance.h"
#include "MDFastBindingObject.h"
#include "SMDFastBindingEditorWidget.h"
#include "PropertyEditorDelegates.h"
#include "PropertyEditorModule.h"
#include "UMGEditorModule.h"
#include "Framework/Docking/LayoutExtender.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Modules/ModuleManager.h"
#include "WidgetBlueprint.h"
#include "WidgetDrawerConfig.h"
#include "BlueprintModes/WidgetBlueprintApplicationMode.h"
#include "Framework/Application/SlateApplication.h"
#include "Util/MDFastBindingEditorHelpers.h"
#include "Widgets/Docking/SDockTab.h"
#include "WorkflowOrientedApp/WorkflowTabManager.h"

#define LOCTEXT_NAMESPACE "FMDFastBindingEditorModule"

FMDFastBindingEditorTabBinding::FMDFastBindingEditorTabBinding()
{
	FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");
	BlueprintEditorTabSpawnerHandle = BlueprintEditorModule.OnRegisterTabsForEditor().AddRaw(this, &FMDFastBindingEditorTabBinding::RegisterBlueprintEditorTab);
	BlueprintEditorLayoutExtensionHandle = BlueprintEditorModule.OnRegisterLayoutExtensions().AddRaw(this, &FMDFastBindingEditorTabBinding::RegisterBlueprintEditorLayout);
}

FMDFastBindingEditorTabBinding::~FMDFastBindingEditorTabBinding()
{
	if (FBlueprintEditorModule* BlueprintEditorModule = FModuleManager::GetModulePtr<FBlueprintEditorModule>("Kismet"))
	{
		BlueprintEditorModule->OnRegisterTabsForEditor().Remove(BlueprintEditorTabSpawnerHandle);
		BlueprintEditorModule->OnRegisterLayoutExtensions().Remove(BlueprintEditorLayoutExtensionHandle);
	}
}

void FMDFastBindingEditorTabBinding::RegisterBlueprintEditorLayout(FLayoutExtender& Extender)
{
	Extender.ExtendLayout(FBlueprintEditorTabs::GraphEditorID, ELayoutExtensionPosition::Before, FTabManager::FTab(FMDFastBindingEditorSummoner::TabId, ETabState::ClosedTab));
}

void FMDFastBindingEditorTabBinding::RegisterBlueprintEditorTab(FWorkflowAllowedTabSet& TabFactories, FName InModeName, TSharedPtr<FBlueprintEditor> BlueprintEditor)
{
	TabFactories.RegisterFactory(MakeShared<FMDFastBindingEditorSummoner>(BlueprintEditor));
}

void FMDFastBindingEditorModule::StartupModule()
{
	FMDFastBindingEditorStyle::Initialize();

	TabBinding = MakeShared<FMDFastBindingEditorTabBinding>();

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomPropertyTypeLayout(FMDFastBindingFieldPath::StaticStruct()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FMDFastBindingFieldPathCustomization::MakeInstance));
	PropertyModule.RegisterCustomPropertyTypeLayout(FMDFastBindingFunctionWrapper::StaticStruct()->GetFName(), FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FMDFastBindingFunctionWrapperCustomization::MakeInstance));
	PropertyModule.RegisterCustomClassLayout(UMDFastBindingObject::StaticClass()->GetFName(), FOnGetDetailCustomizationInstance::CreateStatic(&FMDFastBindingObjectCustomization::MakeInstance));

	FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");
	BlueprintEditorModule.GetMenuExtensibilityManager()->GetExtenderDelegates().Add(FAssetEditorExtender::CreateRaw(this, &FMDFastBindingEditorModule::CheckAddBindingEditorToolbarButtons));

	DesignerExtensionFactory = FMDFastBindingDesignerExtension::MakeFactory();
	IUMGEditorModule& UMGEditorInterface = FModuleManager::GetModuleChecked<IUMGEditorModule>("UMGEditor");
	UMGEditorInterface.GetDesignerExtensibilityManager()->AddDesignerExtensionFactory(DesignerExtensionFactory.ToSharedRef());
	UMGEditorInterface.OnRegisterTabsForEditor().AddRaw(this, &FMDFastBindingEditorModule::HandleRegisterBlueprintEditorTab);

	PropertyBindingExtension = MakeShared<FMDFastBindingPropertyBindingExtension>();
	UMGEditorInterface.GetPropertyBindingExtensibilityManager()->AddExtension(PropertyBindingExtension.ToSharedRef());

	RenameHandle = FBlueprintEditorUtils::OnRenameVariableReferencesEvent.AddRaw(this, &FMDFastBindingEditorModule::OnRenameVariable);
}

void FMDFastBindingEditorModule::ShutdownModule()
{
	FBlueprintEditorUtils::OnRenameVariableReferencesEvent.Remove(RenameHandle);

	if (FPropertyEditorModule* PropertyModule = FModuleManager::GetModulePtr<FPropertyEditorModule>("PropertyEditor"))
	{
		PropertyModule->UnregisterCustomPropertyTypeLayout(FMDFastBindingFieldPath::StaticStruct()->GetFName());
		PropertyModule->UnregisterCustomPropertyTypeLayout(FMDFastBindingFunctionWrapper::StaticStruct()->GetFName());
		PropertyModule->UnregisterCustomClassLayout(UMDFastBindingObject::StaticClass()->GetFName());
	}

	if (IUMGEditorModule* UMGEditorInterface = FModuleManager::GetModulePtr<IUMGEditorModule>("UMGEditor"))
	{
		UMGEditorInterface->GetDesignerExtensibilityManager()->RemoveDesignerExtensionFactory(DesignerExtensionFactory.ToSharedRef());
		UMGEditorInterface->OnRegisterTabsForEditor().RemoveAll(this);
		UMGEditorInterface->GetPropertyBindingExtensibilityManager()->RemoveExtension(PropertyBindingExtension.ToSharedRef());
	}

	FMDFastBindingEditorStyle::Shutdown();

	PropertyBindingExtension.Reset();
	TabBinding.Reset();
	DesignerExtensionFactory.Reset();
}

TSharedRef<FExtender> FMDFastBindingEditorModule::CheckAddBindingEditorToolbarButtons(
	const TSharedRef<FUICommandList> Commands, const TArray<UObject*> Objects) const
{
	TSharedRef<FExtender> Extender = MakeShareable(new FExtender());

	if (UObject* EditorObject = (Objects.Num() > 0) ? Objects[0] : nullptr)
	{
		if (MDFastBindingEditorHelpers::DoesObjectSupportFastBindings(*EditorObject))
		{
			Extender->AddToolBarExtension(TEXT("Asset"), EExtensionHook::After, Commands
				, FToolBarExtensionDelegate::CreateRaw(this, &FMDFastBindingEditorModule::AddBindingEditorToolbarButtons, MakeWeakObjectPtr(EditorObject)));
		}
	}

	return Extender;
}

void FMDFastBindingEditorModule::AddBindingEditorToolbarButtons(FToolBarBuilder& ToolBarBuilder, TWeakObjectPtr<UObject> EditorObject) const
{
	ToolBarBuilder.AddToolBarButton(
		FUIAction(FExecuteAction::CreateRaw(this, &FMDFastBindingEditorModule::OpenBindingEditor, EditorObject))
		, NAME_None
		, LOCTEXT("BindingEditorButtonLabel", "Binding Editor")
		, LOCTEXT("BindingEditorButtonLabelTooltip", "Opens the binding editor for this asset")
		, FSlateIcon(FMDFastBindingEditorStyle::GetStyleSetName(), TEXT("Icon.FastBinding_24x")));

	if (EditorObject.IsValid() && EditorObject->IsA<UWidgetBlueprint>())
	{
		ToolBarBuilder.AddToolBarButton(
			FUIAction(FExecuteAction::CreateUObject(GetMutableDefault<UMDFastBindingEditorConfig>(), &UMDFastBindingEditorConfig::ToggleShouldRunBindingsAtDesignTime))
			, NAME_None
			, TAttribute<FText>::CreateLambda([]()
			{
				return GetDefault<UMDFastBindingEditorConfig>()->ShouldRunBindingsAtDesignTime()
					? LOCTEXT("DisableDesignTimeBindingsButtonLabel", "Disable Design-Time Bindings")
					: LOCTEXT("EnableDesignTimeBindingsButtonLabel", "Enable Design-Time Bindings");
			})
			, LOCTEXT("ToggleDesignTimeBindingsButtonTooltip", "Toggle whether bindings should execute in the widget designer.")
			, TAttribute<FSlateIcon>::CreateLambda([]()
			{
				return GetDefault<UMDFastBindingEditorConfig>()->ShouldRunBindingsAtDesignTime()
					? FSlateIcon(FMDFastBindingEditorStyle::GetStyleSetName(), TEXT("Icon.Enabled"))
					: FSlateIcon(FMDFastBindingEditorStyle::GetStyleSetName(), TEXT("Icon.Disabled"));
			}));
	}
}

void FMDFastBindingEditorModule::OpenBindingEditor(TWeakObjectPtr<UObject> EditorObject) const
{
	UObject* Object = EditorObject.Get();
	if (Object == nullptr)
	{
		return;
	}

	const UPackage* Package = Object->GetPackage();
	UBlueprint* BP = Cast<UBlueprint>(Package->FindAssetInPackage());
	TArray<IAssetEditorInstance*> Editors = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorsForAsset(BP);
	for (IAssetEditorInstance* Editor : Editors)
	{
		// Force switch to graph mode, in case this is a Widget Blueprint, since the binding editor opens in the graph mode
		if (Editor->GetEditorName() == FName("BlueprintEditor") || Editor->GetEditorName() == FName("WidgetBlueprintEditor"))
		{
			FWorkflowCentricApplication* WorkflowApp = static_cast<FWorkflowCentricApplication*>(Editor);
			if (WorkflowApp->GetCurrentMode() == TEXT("DesignerName"))
			{
				WorkflowApp->SetCurrentMode(TEXT("GraphName"));
			}
		}

		TSharedPtr<FTabManager> TabManager = Editor->GetAssociatedTabManager();
		if (TabManager.IsValid() && TabManager->HasTabSpawner(FMDFastBindingEditorSummoner::TabId))
		{
			if (const TSharedPtr<SDockTab> Tab = TabManager->TryInvokeTab(FMDFastBindingEditorSummoner::TabId))
			{
				const TSharedPtr<SMDFastBindingEditorWidget> BindingWidget = StaticCastSharedRef<SMDFastBindingEditorWidget>(Tab->GetContent());
				BindingWidget->AssignBindingData(BP);
			}
		}
	}
}

void FMDFastBindingEditorModule::OnRenameVariable(UBlueprint* Blueprint, UClass* VariableClass, const FName& OldVariableName, const FName& NewVariableName)
{
	if (Blueprint != nullptr && MDFastBindingEditorHelpers::DoesClassSupportFastBindings(Blueprint->GeneratedClass))
	{
		if (const UMDFastBindingContainer* BindingContainer = MDFastBindingEditorHelpers::FindBindingContainerCDOInClass(Blueprint->GeneratedClass))
		{
			for (UMDFastBindingInstance* Binding : BindingContainer->GetBindings())
			{
				if (Binding != nullptr)
				{
					Binding->OnVariableRenamed(VariableClass, OldVariableName, NewVariableName);
				}
			}
		}
	}
}

void FMDFastBindingEditorModule::HandleRegisterBlueprintEditorTab(const FWidgetBlueprintApplicationMode& ApplicationMode, FWorkflowAllowedTabSet& TabFactories)
{
	if (ApplicationMode.LayoutExtender)
	{
		ApplicationMode.OnPostActivateMode.AddRaw(this, &FMDFastBindingEditorModule::HandleActivateMode);
		ApplicationMode.OnPreDeactivateMode.AddRaw(this, &FMDFastBindingEditorModule::HandleDeactivateMode);
	}
}

void FMDFastBindingEditorModule::HandleActivateMode(FWidgetBlueprintApplicationMode& InDesignerMode)
{
	if (TSharedPtr<FWidgetBlueprintEditor> BP = InDesignerMode.GetBlueprintEditor())
	{
		if (!BP->GetExternalEditorWidget(FMDFastBindingEditorSummoner::DrawerId))
		{
			const FMDFastBindingEditorSummoner BindingDrawerSummoner(BP);
			const FWorkflowTabSpawnInfo SpawnInfo;
			BP->AddExternalEditorWidget(FMDFastBindingEditorSummoner::DrawerId, BindingDrawerSummoner.CreateTabBody(SpawnInfo));
		}

		FWidgetDrawerConfig BindingDrawer(FMDFastBindingEditorSummoner::DrawerId);
		TWeakPtr<FWidgetBlueprintEditor> WeakBP = BP;
		BindingDrawer.GetDrawerContentDelegate.BindLambda([WeakBP]()
		{
			if (TSharedPtr<FWidgetBlueprintEditor> BP = WeakBP.Pin())
			{
				if (const TSharedPtr<SWidget> DrawerWidgetContent = BP->GetExternalEditorWidget(FMDFastBindingEditorSummoner::DrawerId))
				{
					return DrawerWidgetContent.ToSharedRef();
				}
			}

			return SNullWidget::NullWidget;
		});
		BindingDrawer.OnDrawerOpenedDelegate.BindLambda([WeakBP](FName StatusBarWithDrawerName)
		{
			if (TSharedPtr<FWidgetBlueprintEditor> BP = WeakBP.Pin())
			{
				FSlateApplication::Get().SetUserFocus(FSlateApplication::Get().GetUserIndexForKeyboard(), BP->GetExternalEditorWidget(FMDFastBindingEditorSummoner::DrawerId));
			}
		});
		BindingDrawer.OnDrawerDismissedDelegate.BindLambda([WeakBP](const TSharedPtr<SWidget>& NewlyFocusedWidget)
		{
			if (TSharedPtr<FWidgetBlueprintEditor> BP = WeakBP.Pin())
			{
				BP->SetKeyboardFocus();
			}
		});
		BindingDrawer.ButtonText = LOCTEXT("BindingEditor", "Binding Editor");
		BindingDrawer.ToolTipText = LOCTEXT("BindingEditorToolTip", "Opens the binding editor drawer for this asset");
		BindingDrawer.Icon = FMDFastBindingEditorStyle::Get().GetBrush(TEXT("Icon.FastBinding_16x"));
		BP->RegisterDrawer(MoveTemp(BindingDrawer), INDEX_NONE);
	}
}

void FMDFastBindingEditorModule::HandleDeactivateMode(FWidgetBlueprintApplicationMode& InDesignerMode)
{
	TSharedPtr<FWidgetBlueprintEditor> BP = InDesignerMode.GetBlueprintEditor();
	if (BP && BP->IsEditorClosing())
	{
		InDesignerMode.OnPostActivateMode.RemoveAll(this);
		InDesignerMode.OnPreDeactivateMode.RemoveAll(this);
	}
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FMDFastBindingEditorModule, MDFastBindingEditor)
