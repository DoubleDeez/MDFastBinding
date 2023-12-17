#include "BlueprintExtension/MDFastBindingWidgetBlueprintExtension.h"

#include "Graph/MDFastBindingGraph.h"
#include "Graph/MDFastBindingGraphSchema.h"
#include "MDFastBindingContainer.h"
#include "MDFastBindingHelpers.h"
#include "MDFastBindingInstance.h"
#include "WidgetExtension/MDFastBindingWidgetClassExtension.h"

void UMDFastBindingWidgetBlueprintExtension::SetBindingContainer(UMDFastBindingContainer* InContainer)
{
	check(InContainer && InContainer->GetOuter() == this);
	BindingContainer = InContainer;
}

bool UMDFastBindingWidgetBlueprintExtension::DoesBlueprintOrSuperClassesHaveBindings() const
{
	if (BindingContainer != nullptr && BindingContainer->HasBindings())
	{
		return true;
	}

	if (const UWidgetBlueprint* WidgetBP = GetWidgetBlueprint())
	{
		return FMDFastBindingHelpers::DoesClassHaveSuperClassBindings(Cast<UWidgetBlueprintGeneratedClass>(WidgetBP->GeneratedClass.Get()));
	}

	return false;
}

#if WITH_EDITORONLY_DATA
void UMDFastBindingWidgetBlueprintExtension::GetAllGraphs(TArray<UEdGraph*>& Graphs) const
{
#if defined(WITH_FASTBINDING_DIFFS) && WITH_FASTBINDING_DIFFS
	Super::GetAllGraphs(Graphs);
#endif

	Graphs.Append(PinnedGraphs);
}
#endif

void UMDFastBindingWidgetBlueprintExtension::HandleBeginCompilation(FWidgetBlueprintCompilerContext& InCreationContext)
{
	Super::HandleBeginCompilation(InCreationContext);

	PinnedGraphs.Empty();

	CompilerContext = &InCreationContext;
}

void UMDFastBindingWidgetBlueprintExtension::HandleCopyTermDefaultsToDefaultObject(UObject* DefaultObject)
{
	Super::HandleCopyTermDefaultsToDefaultObject(DefaultObject);
	
	if (UWidgetBlueprintGeneratedClass* WidgetBPClass = Cast<UWidgetBlueprintGeneratedClass>(DefaultObject->GetClass()))
	{
		if (CompilerContext != nullptr && DoesBlueprintOrSuperClassesHaveBindings())
		{
			UMDFastBindingWidgetClassExtension* BindingClass = NewObject<UMDFastBindingWidgetClassExtension>(WidgetBPClass);
			if (BindingContainer != nullptr && BindingContainer->GetBindings().Num() > 0)
			{
				BindingClass->SetBindingContainer(BindingContainer);
			}
		
			// There's a chance we could perform some compile-time steps here that would improve runtime performance
			CompilerContext->AddExtension(WidgetBPClass, BindingClass);

			// The blueprint has been fully recompiled here, we need to update the binding graphs
			RefreshPinnedGraphs();
		}
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

	if (CompilerContext)
	{
		// If this class already has the class extension serialized, it had already been saved prior
		// There's a chance it's being freshly loaded here so we need to generate the binding graphs
		if (UWidgetBlueprintGeneratedClass* WidgetBPClass = Cast<UWidgetBlueprintGeneratedClass>(CompilerContext->NewClass))
		{
			if (WidgetBPClass->GetExtension<UMDFastBindingWidgetClassExtension>() != nullptr)
			{
				RefreshPinnedGraphs();
			}
		}

		CompilerContext = nullptr;
	}
}

void UMDFastBindingWidgetBlueprintExtension::RefreshPinnedGraphs()
{
	if (PinnedGraphs.IsEmpty() && BindingContainer != nullptr)
	{	
		// Since we don't serialize the binding graphs, we need to generate them on the fly
		for (UMDFastBindingInstance* BindingInstance : BindingContainer->GetBindings())
		{
			const FString GraphName = FString::Printf(TEXT("FastBinding: %s"), *BindingInstance->GetBindingDisplayName().ToString());
			UMDFastBindingGraph* GraphObj = NewObject<UMDFastBindingGraph>(GetWidgetBlueprint(), FName(*GraphName), RF_Transient);
			GraphObj->Schema = UMDFastBindingGraphSchema::StaticClass();
			GraphObj->SetBinding(BindingInstance);
			PinnedGraphs.Add(GraphObj);
		}
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
