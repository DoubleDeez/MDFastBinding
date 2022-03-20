#include "BindingDestinations/MDFastBindingDestinationBase.h"

#include "MDFastBindingInstance.h"
#include "BindingValues/MDFastBindingValueBase.h"

void UMDFastBindingDestinationBase::InitializeDestination(UObject* SourceObject)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(__FUNCTION__);
	TRACE_CPUPROFILER_EVENT_SCOPE_TEXT(*GetName());
	InitializeDestination_Internal(SourceObject);

	for (const FMDFastBindingItem& BindingItem : BindingItems)
	{
		if (BindingItem.Value != nullptr)
		{
			BindingItem.Value->InitializeValue(SourceObject);
		}
	}
}

void UMDFastBindingDestinationBase::UpdateDestination(UObject* SourceObject)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(__FUNCTION__);
	TRACE_CPUPROFILER_EVENT_SCOPE_TEXT(*GetName());
	if (CheckNeedsUpdate())
	{
		UpdateDestination_Internal(SourceObject);
		bHasEverUpdated = true;
	}
}

void UMDFastBindingDestinationBase::TerminateDestination(UObject* SourceObject)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(__FUNCTION__);
	TRACE_CPUPROFILER_EVENT_SCOPE_TEXT(*GetName());
	TerminateDestination_Internal(SourceObject);
}

bool UMDFastBindingDestinationBase::CheckNeedsUpdate() const
{
	return !bHasEverUpdated || Super::CheckNeedsUpdate();
}

#if WITH_EDITOR
bool UMDFastBindingDestinationBase::IsActive() const
{
	if (const UMDFastBindingInstance* Binding = GetOuterBinding())
	{
		return Binding->GetBindingDestination() == this;
	}

	return false;
}
#endif
