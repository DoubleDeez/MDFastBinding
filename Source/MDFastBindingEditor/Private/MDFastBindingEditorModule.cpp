#include "MDFastBindingEditorModule.h"

#include "BlueprintEditorModule.h"
#include "BlueprintEditorTabs.h"
#include "LevelEditor.h"
#include "MDFastBindingContainer.h"
#include "MDFastBindingEditorStyle.h"
#include "MDFastBindingFieldPath.h"
#include "MDFastBindingFieldPathCustomization.h"
#include "MDFastBindingFunctionWrapper.h"
#include "MDFastBindingFunctionWrapperCustomization.h"
#include "SMDFastBindingEditorWidget.h"
#include "PropertyEditorDelegates.h"
#include "PropertyEditorModule.h"
#include "Framework/Docking/LayoutExtender.h"
#include "Modules/ModuleManager.h"
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

void FMDFastBindingEditorTabBinding::RegisterBlueprintEditorTab(FWorkflowAllowedTabSet& TabFactories, FName InModeName,
	TSharedPtr<FBlueprintEditor> BlueprintEditor)
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
	
	FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");
	BlueprintEditorModule.GetMenuExtensibilityManager()->GetExtenderDelegates().Add(FAssetEditorExtender::CreateRaw(this, &FMDFastBindingEditorModule::CheckAddBindingEditorToolbarButton));
}

void FMDFastBindingEditorModule::ShutdownModule()
{
	if (FPropertyEditorModule* PropertyModule = FModuleManager::GetModulePtr<FPropertyEditorModule>("PropertyEditor"))
	{
		PropertyModule->UnregisterCustomPropertyTypeLayout(FMDFastBindingFieldPath::StaticStruct()->GetFName());
		PropertyModule->UnregisterCustomPropertyTypeLayout(FMDFastBindingFunctionWrapper::StaticStruct()->GetFName());
	}
	
	FMDFastBindingEditorStyle::Shutdown();

	TabBinding.Reset();
}

TSharedRef<FExtender> FMDFastBindingEditorModule::CheckAddBindingEditorToolbarButton(
	const TSharedRef<FUICommandList> Commands, const TArray<UObject*> Objects) const
{
	TSharedRef<FExtender> Extender = MakeShareable(new FExtender());

	if (UObject* EditorObject = (Objects.Num() > 0) ? Objects[0] : nullptr)
	{
		if (DoesObjectHaveFastBindings(*EditorObject))
		{
			Extender->AddToolBarExtension(TEXT("Asset"), EExtensionHook::After, Commands
				, FToolBarExtensionDelegate::CreateRaw(
					this, &FMDFastBindingEditorModule::AddBindingEditorToolbarButton, MakeWeakObjectPtr(EditorObject)));
		}	
	}
	
	return Extender;	
}

void FMDFastBindingEditorModule::AddBindingEditorToolbarButton(FToolBarBuilder& ToolBarBuilder, TWeakObjectPtr<UObject> EditorObject) const
{
	ToolBarBuilder.AddToolBarButton(
		FUIAction(FExecuteAction::CreateRaw(this, &FMDFastBindingEditorModule::OpenBindingEditor, EditorObject))
		, NAME_None
		, LOCTEXT("BindingEditorButtonLabel", "Binding Editor")
		, LOCTEXT("BindingEditorButtonLabel", "Opens the binding editor for this asset")
		, FSlateIcon(FMDFastBindingEditorStyle::GetStyleSetName(), TEXT("Icon.FastBinding_24x")));
}

bool FMDFastBindingEditorModule::DoesObjectHaveFastBindings(const UObject& Object)
{
	if (const UBlueprint* BP = Cast<UBlueprint>(&Object))
	{
		return DoesClassHaveFastBindings(BP->GeneratedClass);
	}

	return DoesClassHaveFastBindings(Object.GetClass());
}

bool FMDFastBindingEditorModule::DoesClassHaveFastBindings(const UStruct* Class)
{
	TSet<const UStruct*> VisitedClasses;
	TArray<const UStruct*> ClassesToVisit = { Class };

	while (ClassesToVisit.Num() > 0)
	{
		const UStruct* ClassToVisit = ClassesToVisit.Pop();
		VisitedClasses.Add(ClassToVisit);
		
		for (TFieldIterator<FProperty> It(ClassToVisit); It; ++It)
		{
			if (const FObjectPropertyBase* ObjectProp = CastField<const FObjectPropertyBase>(*It))
			{
				if (ObjectProp->PropertyClass->IsChildOf(UMDFastBindingContainer::StaticClass()))
				{
					return true;
				}
				
				if (!VisitedClasses.Contains(ObjectProp->PropertyClass))
				{
					ClassesToVisit.Push(ObjectProp->PropertyClass);
				}
			}
			else if (const FStructProperty* StructProp = CastField<const FStructProperty>(*It))
			{
				if (!VisitedClasses.Contains(StructProp->Struct))
				{
					ClassesToVisit.Push(StructProp->Struct);
				}
			}
		}
	}
	
	return false;
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
				BindingWidget->AssignBindingData(BP->GeneratedClass);
			}
		}
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FMDFastBindingEditorModule, MDFastBindingEditor)
