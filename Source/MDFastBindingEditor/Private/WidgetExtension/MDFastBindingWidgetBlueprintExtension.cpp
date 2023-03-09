#include "WidgetExtension/MDFastBindingWidgetBlueprintExtension.h"

#include "MDFastBindingContainer.h"
#include "MDFastBindingInstance.h"
#include "Graph/MDFastBindingGraph.h"
#include "Graph/MDFastBindingGraphSchema.h"
#include "WidgetExtension/MDFastBindingWidgetClassExtension.h"

void UMDFastBindingWidgetBlueprintExtension::SetBindingContainer(UMDFastBindingContainer* InContainer)
{
	check(InContainer && InContainer->GetOuter() == this);
	BindingContainer = InContainer;
	BindContainerOwnerDelegate();
}

void UMDFastBindingWidgetBlueprintExtension::PostLoad()
{
	Super::PostLoad();
	
	BindContainerOwnerDelegate();
}

#if WITH_EDITORONLY_DATA
void UMDFastBindingWidgetBlueprintExtension::GetAllGraphs(TArray<UEdGraph*>& Graphs) const
{
#if defined(WITH_FASTBINDING_DIFFS) && WITH_FASTBINDING_DIFFS
	Super::GetAllGraphs(Graphs);
#endif

	// Since we don't serialize the binding graphs, we need to generate them on the fly
	if (PinnedGraphs.IsEmpty() && BindingContainer != nullptr)
	{
		for (UMDFastBindingInstance* BindingInstance : BindingContainer->GetBindings())
		{
			const FString GraphName = FString::Printf(TEXT("FastBinding: %s"), *BindingInstance->GetBindingDisplayName().ToString());
			UMDFastBindingGraph* GraphObj = NewObject<UMDFastBindingGraph>(GetWidgetBlueprint(), FName(*GraphName), RF_Transient);
			GraphObj->Schema = UMDFastBindingGraphSchema::StaticClass();
			GraphObj->SetBinding(BindingInstance);
			PinnedGraphs.Add(GraphObj);
		}
	}

	Graphs.Append(PinnedGraphs);
}
#endif

void UMDFastBindingWidgetBlueprintExtension::HandleBeginCompilation(FWidgetBlueprintCompilerContext& InCreationContext)
{
	Super::HandleBeginCompilation(InCreationContext);
	
	PinnedGraphs.Empty();

	CompilerContext = &InCreationContext;
}

void UMDFastBindingWidgetBlueprintExtension::HandleFinishCompilingClass(UWidgetBlueprintGeneratedClass* Class)
{
	Super::HandleFinishCompilingClass(Class);
	
	if (CompilerContext != nullptr && BindingContainer != nullptr && BindingContainer->GetBindings().Num() > 0)
	{
		UMDFastBindingWidgetClassExtension* BindingClass = NewObject<UMDFastBindingWidgetClassExtension>(Class);
		BindingClass->SetBindingContainer(BindingContainer);

		// There's a chance we could perform some compile-time steps here that would improve runtime performance
		
		CompilerContext->AddExtension(Class, BindingClass);
	}
}

bool UMDFastBindingWidgetBlueprintExtension::HandleValidateGeneratedClass(UWidgetBlueprintGeneratedClass* Class)
{
	if (BindingContainer == nullptr)
	{
		return false;
	}

	TArray<FText> ValidationErrors;
	if (BindingContainer->IsDataValid(ValidationErrors) == EDataValidationResult::Invalid)
	{
		return false;
	}
	
	return Super::HandleValidateGeneratedClass(Class);
}

void UMDFastBindingWidgetBlueprintExtension::HandleEndCompilation()
{
	Super::HandleEndCompilation();

	CompilerContext = nullptr;
}

void UMDFastBindingWidgetBlueprintExtension::BindContainerOwnerDelegate()
{
	if (BindingContainer != nullptr)
	{
		BindingContainer->GetBindingOwnerClassDelegate.BindUObject(this, &UMDFastBindingWidgetBlueprintExtension::GetBindingOwnerClass);
	}
}

UClass* UMDFastBindingWidgetBlueprintExtension::GetBindingOwnerClass() const
{
	if (const UWidgetBlueprint* WidgetBP = GetWidgetBlueprint())
	{
		return WidgetBP->GeneratedClass;
	}
	
	return nullptr;
}
