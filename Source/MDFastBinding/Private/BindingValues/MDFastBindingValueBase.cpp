#include "BindingValues/MDFastBindingValueBase.h"

void UMDFastBindingValueBase::BeginDestroy()
{
	Super::BeginDestroy();

	if (CachedValue.Value != nullptr)
	{
		FMemory::Free(CachedValue.Value);
		CachedValue.Value = nullptr;
	}
}

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

	InitializeValue_Internal(SourceObject);
}

void UMDFastBindingValueBase::TerminateValue(UObject* SourceObject)
{
	TerminateValue_Internal(SourceObject);
}

TTuple<const FProperty*, void*> UMDFastBindingValueBase::GetValue(UObject* SourceObject, bool& OutDidUpdate)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(__FUNCTION__);
	TRACE_CPUPROFILER_EVENT_SCOPE_TEXT(*GetName());

	OutDidUpdate = false;

	if (CheckCachedNeedsUpdate())
	{
		const TTuple<const FProperty*, void*> Value = GetValue_Internal(SourceObject);
		if (Value.Key == nullptr || Value.Value == nullptr)
		{
			return Value;
		}

		if (CachedValue.Key == nullptr || CachedValue.Value == nullptr)
		{
			CachedValue.Key = Value.Key;
			CachedValue.Value = FMemory::Malloc(CachedValue.Key->GetSize(), CachedValue.Key->GetMinAlignment());
			CachedValue.Key->InitializeValue(CachedValue.Value);
			CachedValue.Key->CopyCompleteValue(CachedValue.Value, Value.Value);
			OutDidUpdate = true;
		}
		else if (!CachedValue.Key->Identical(CachedValue.Value, Value.Value))
		{
			CachedValue.Key->CopyCompleteValue(CachedValue.Value, Value.Value);
			OutDidUpdate = true;
		}

		MarkObjectClean();
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
