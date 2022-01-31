#include "MDFastBindingContainer.h"
#include "BindingDestinations/MDFastBindingDestinationBase.h"

void UMDFastBindingContainer::InitializeBindings(UObject* SourceObject)
{
	for (UMDFastBindingDestinationBase* Destination : Destinations)
	{
		Destination->InitializeDestination(SourceObject);
	}
}

void UMDFastBindingContainer::UpdateBindings(UObject* SourceObject)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(__FUNCTION__);
	TRACE_CPUPROFILER_EVENT_SCOPE_TEXT(*GetNameSafe(SourceObject));
	for (UMDFastBindingDestinationBase* Destination : Destinations)
	{
		Destination->UpdateDestination(SourceObject);
	}
}

void UMDFastBindingContainer::TerminateBindings(UObject* SourceObject)
{
	for (UMDFastBindingDestinationBase* Destination : Destinations)
	{
		Destination->TerminateDestination(SourceObject);
	}
}

#if WITH_EDITORONLY_DATA
UMDFastBindingDestinationBase* UMDFastBindingContainer::AddBinding(TSubclassOf<UMDFastBindingDestinationBase> BindingClass)
{
	if (BindingClass != nullptr)
	{
		UMDFastBindingDestinationBase* Binding = NewObject<UMDFastBindingDestinationBase>(this, BindingClass, NAME_None, RF_Public | RF_Transactional);
		Destinations.Add(Binding);
		return Binding;
	}

	return nullptr;
}

UMDFastBindingDestinationBase* UMDFastBindingContainer::DuplicateBinding(UMDFastBindingDestinationBase* InBinding)
{
	const int32 CurrentIdx = Destinations.IndexOfByKey(InBinding);
	if (CurrentIdx != INDEX_NONE)
	{
		if (UMDFastBindingDestinationBase* NewBinding = DuplicateObject<UMDFastBindingDestinationBase>(InBinding, this, NAME_None))
		{
			Destinations.Insert(NewBinding, CurrentIdx + 1);
			return NewBinding;
		}
	}

	return nullptr;
}

bool UMDFastBindingContainer::DeleteBinding(UMDFastBindingDestinationBase* InBinding)
{
	if (Destinations.Contains(InBinding))
	{
		Destinations.Remove(InBinding);
		return true;
	}

	return false;
}
#endif

#if WITH_EDITOR
EDataValidationResult UMDFastBindingContainer::IsDataValid(TArray<FText>& ValidationErrors)
{
	EDataValidationResult Result = EDataValidationResult::Valid;

	for (UMDFastBindingDestinationBase* Destination : Destinations)
	{
		if (Destination != nullptr)
		{
			if (Destination->IsDataValid(ValidationErrors) == EDataValidationResult::Invalid)
			{
				Result = EDataValidationResult::Invalid;
			}
		}
	}

	return Result;
}
#endif
