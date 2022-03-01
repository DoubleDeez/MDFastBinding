#include "BindingValues/MDFastBindingValueBase.h"

TTuple<const FProperty*, void*> UMDFastBindingValueBase::GetValue(UObject* SourceObject, bool& OutDidUpdate)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(__FUNCTION__);
	TRACE_CPUPROFILER_EVENT_SCOPE_TEXT(*GetName());
	
	OutDidUpdate = false;
	
	if (CheckNeedsUpdate())
	{
		const TTuple<const FProperty*, void*> Value = GetValue_Internal(SourceObject);
		if (Value.Key == nullptr || Value.Value == nullptr)
		{
			return Value;
		}

		if (CachedValue.Key == nullptr || CachedValue.Value == nullptr || !CachedValue.Key->Identical(CachedValue.Value, Value.Value))
		{
			CachedValue.Key = Value.Key;
			CachedValue.Value = FMemory::Malloc(CachedValue.Key->GetSize(), CachedValue.Key->GetMinAlignment());
			CachedValue.Key->InitializeValue(CachedValue.Value);
			CachedValue.Key->CopyCompleteValue(CachedValue.Value, Value.Value);
			OutDidUpdate = true;
		}
	}
	
	return CachedValue;
}

bool UMDFastBindingValueBase::CheckNeedsUpdate() const
{
	return CachedValue.Value == nullptr || Super::CheckNeedsUpdate();
}
