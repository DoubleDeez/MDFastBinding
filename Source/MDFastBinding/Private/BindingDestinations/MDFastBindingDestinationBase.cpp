#include "BindingDestinations/MDFastBindingDestinationBase.h"

#include "MDFastBindingInstance.h"
#include "BindingValues/MDFastBindingValueBase.h"

void UMDFastBindingDestinationBase::InitializeDestination(UObject* SourceObject)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(__FUNCTION__);
	InitializeDestination_Internal(SourceObject);
}

void UMDFastBindingDestinationBase::UpdateDestination(UObject* SourceObject)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(__FUNCTION__);
	UpdateDestination_Internal(SourceObject);
}

void UMDFastBindingDestinationBase::TerminateDestination(UObject* SourceObject)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(__FUNCTION__);
	TerminateDestination_Internal(SourceObject);
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
