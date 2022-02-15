#include "BindingDestinations/MDFastBindingDestinationBase.h"
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

#if WITH_EDITORONLY_DATA
UMDFastBindingValueBase* UMDFastBindingDestinationBase::AddOrphan(UMDFastBindingValueBase* InValue)
{
	if (InValue != nullptr && !OrphanedValues.Contains(InValue))
	{
		Modify();
		UMDFastBindingValueBase* Value = DuplicateObject(InValue, this);
		OrphanedValues.Add(Value);
		return Value;
	}

	return nullptr;
}

void UMDFastBindingDestinationBase::RemoveOrphan(UMDFastBindingValueBase* InValue)
{
	Modify();
	OrphanedValues.Remove(InValue);
}
#endif
