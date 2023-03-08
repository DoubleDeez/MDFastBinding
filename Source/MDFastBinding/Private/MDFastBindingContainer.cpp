#include "MDFastBindingContainer.h"
#include "MDFastBindingInstance.h"
#include "MDFastBindingLog.h"
#include "BindingDestinations/MDFastBindingDestinationBase.h"
#include "Blueprint/UserWidget.h"

void UMDFastBindingContainer::InitializeBindings(UObject* SourceObject)
{
	if (const UUserWidget* OuterWidget = Cast<UUserWidget>(GetOuter()))
	{
		UE_CLOG(!OuterWidget->IsDesignTime(), LogMDFastBinding, Warning, TEXT("[%s] uses a deprecated property-based MDFastBindingContainer, resave it to automatically upgrade it to a widget extension"), *GetNameSafe(OuterWidget->GetClass()));
	}
	
	for (int32 i = 0; i < Bindings.Num(); ++i)
	{
		if (UMDFastBindingInstance* Binding = Bindings[i])
		{
			Binding->InitializeBinding(SourceObject);
			Binding->UpdateBinding(SourceObject);
		
			BindingTickPolicyLookUpMap.Add(i, Binding->ShouldBindingTick());
		}
	}
}

void UMDFastBindingContainer::UpdateBindings(UObject* SourceObject)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_STR(__FUNCTION__);
	TRACE_CPUPROFILER_EVENT_SCOPE_TEXT(*GetNameSafe(SourceObject));
	for (int32 i = 0; i < Bindings.Num(); ++i)
	{
		if (bool& bShouldBindingUpdate = BindingTickPolicyLookUpMap.FindOrAdd(i))
		{
			if (UMDFastBindingInstance* Binding = Bindings[i])
			{
				Binding->UpdateBinding(SourceObject);
				bShouldBindingUpdate = Binding->ShouldBindingTick();
			}
		}
	}
}

void UMDFastBindingContainer::TerminateBindings(UObject* SourceObject)
{
	for (UMDFastBindingInstance* Binding : Bindings)
	{
		Binding->TerminateBinding(SourceObject);
	}

	BindingTickPolicyLookUpMap.Empty();
}

void UMDFastBindingContainer::SetBindingTickPolicy(UMDFastBindingInstance* Binding, bool bShouldTick)
{
	const int32 BindingIndex = Bindings.IndexOfByKey(Binding);

	if (BindingIndex != INDEX_NONE)
	{
		BindingTickPolicyLookUpMap.FindOrAdd(BindingIndex, bShouldTick);
	}
}

UClass* UMDFastBindingContainer::GetBindingOwnerClass() const
{
	if (GetBindingOwnerClassDelegate.IsBound())
	{
		return GetBindingOwnerClassDelegate.Execute();
	}
	
	if (const UObject* Outer = GetOuter())
	{
		return Outer->GetClass();
	}
	
	return nullptr;
}

#if WITH_EDITORONLY_DATA
UMDFastBindingInstance* UMDFastBindingContainer::AddBinding()
{
	UMDFastBindingInstance* Binding = NewObject<UMDFastBindingInstance>(this, NAME_None, RF_Public | RF_Transactional);
	Bindings.Add(Binding);
	return Binding;
}

UMDFastBindingInstance* UMDFastBindingContainer::DuplicateBinding(UMDFastBindingInstance* InBinding)
{
	const int32 CurrentIdx = Bindings.IndexOfByKey(InBinding);
	if (CurrentIdx != INDEX_NONE)
	{
		if (UMDFastBindingInstance* NewBinding = DuplicateObject<UMDFastBindingInstance>(InBinding, this, NAME_None))
		{
			// Init unique identifiers for all of the objects
			for (UMDFastBindingObject* Object : NewBinding->GatherAllBindingObjects())
			{
				if (Object != nullptr)
				{
					Object->BindingObjectIdentifier = FGuid::NewGuid();
				}
			}
			
			Bindings.Insert(NewBinding, CurrentIdx + 1);
			return NewBinding;
		}
	}

	return nullptr;
}

bool UMDFastBindingContainer::DeleteBinding(UMDFastBindingInstance* InBinding)
{
	return Bindings.Remove(InBinding) > 0;
}

void UMDFastBindingContainer::MoveBindingToIndex(UMDFastBindingInstance* InBinding, int32 Index)
{
	const int32 CurrentIndex = Bindings.IndexOfByKey(InBinding);
	if (CurrentIndex != INDEX_NONE && Index != INDEX_NONE && CurrentIndex != Index)
	{
		Modify();
		Bindings.Insert(InBinding, Index);
		if (Index > CurrentIndex)
		{
			Bindings.RemoveAt(CurrentIndex);
		}
		else
		{
			Bindings.RemoveAt(CurrentIndex + 1);
		}
	}
}
#endif

#if WITH_EDITOR
EDataValidationResult UMDFastBindingContainer::IsDataValid(TArray<FText>& ValidationErrors)
{
	EDataValidationResult Result = EDataValidationResult::Valid;

	for (UMDFastBindingInstance* Binding : Bindings)
	{
		if (Binding != nullptr)
		{
			if (Binding->IsDataValid(ValidationErrors) == EDataValidationResult::Invalid)
			{
				Result = EDataValidationResult::Invalid;
			}
		}
	}

	return Result;
}
#endif
