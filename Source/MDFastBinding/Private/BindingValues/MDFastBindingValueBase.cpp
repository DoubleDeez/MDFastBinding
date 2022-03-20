#include "BindingValues/MDFastBindingValueBase.h"

void UMDFastBindingValueBase::InitializeValue(UObject* SourceObject)
{
	SetupBindingItems_Internal();

	for (const FMDFastBindingItem& BindingItem : BindingItems)
	{
		if (BindingItem.Value != nullptr)
		{
			BindingItem.Value->InitializeValue(SourceObject);
		}
	}
}

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

const FMDFastBindingItem* UMDFastBindingValueBase::GetOwningBindingItem() const
{
	if (const UMDFastBindingObject* OuterObject = Cast<UMDFastBindingObject>(GetOuter()))
	{
		return OuterObject->FindBindingItemWithValue(this);
	}

	return nullptr;
}
