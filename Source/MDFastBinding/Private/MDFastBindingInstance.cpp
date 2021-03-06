#include "MDFastBindingInstance.h"

#include "MDFastBindingContainer.h"
#include "BindingDestinations/MDFastBindingDestinationBase.h"
#include "BindingValues/MDFastBindingValueBase.h"

UClass* UMDFastBindingInstance::GetBindingOuterClass() const
{
	const UObject* Object = this;
	while (Object != nullptr)
	{
		if (Object->IsA<UMDFastBindingContainer>() && Object->GetOuter() != nullptr)
		{
			return Object->GetOuter()->GetClass();
		}

		Object = Object->GetOuter();
	}

	return nullptr;
}

void UMDFastBindingInstance::InitializeBinding(UObject* SourceObject)
{
	if (BindingDestination != nullptr)
	{
		BindingDestination->InitializeDestination(SourceObject);
	}
}

void UMDFastBindingInstance::UpdateBinding(UObject* SourceObject)
{
	if (BindingDestination != nullptr)
	{
		BindingDestination->UpdateDestination(SourceObject);
	}
}

void UMDFastBindingInstance::TerminateBinding(UObject* SourceObject)
{
	if (BindingDestination != nullptr)
	{
		BindingDestination->TerminateDestination(SourceObject);
	}
}

#if WITH_EDITOR
EDataValidationResult UMDFastBindingInstance::IsDataValid(TArray<FText>& ValidationErrors)
{
	if (BindingDestination != nullptr)
	{
		return BindingDestination->IsDataValid(ValidationErrors);
	}
	
	return UObject::IsDataValid(ValidationErrors);
}

void UMDFastBindingInstance::OnVariableRenamed(UClass* VariableClass, const FName& OldVariableName, const FName& NewVariableName)
{
	if (BindingDestination != nullptr)
	{
		BindingDestination->OnVariableRenamed(VariableClass, OldVariableName, NewVariableName);
	}
	
	for (UMDFastBindingValueBase* Orphan : OrphanedValues)
	{
		if (Orphan != nullptr)
		{
			Orphan->OnVariableRenamed(VariableClass, OldVariableName, NewVariableName);
		}
	}
	
	for (UMDFastBindingDestinationBase* Destination : InactiveDestinations)
	{
		if (Destination != nullptr)
		{
			Destination->OnVariableRenamed(VariableClass, OldVariableName, NewVariableName);
		}
	}
}
#endif

#if WITH_EDITORONLY_DATA
UMDFastBindingValueBase* UMDFastBindingInstance::AddOrphan(UMDFastBindingValueBase* InValue)
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

void UMDFastBindingInstance::RemoveOrphan(UMDFastBindingValueBase* InValue)
{
	Modify();
	OrphanedValues.Remove(InValue);
}

UMDFastBindingDestinationBase* UMDFastBindingInstance::SetDestination(TSubclassOf<UMDFastBindingDestinationBase> InClass)
{
	return SetDestination(NewObject<UMDFastBindingDestinationBase>(this, InClass, NAME_None, RF_Public | RF_Transactional));
}

UMDFastBindingDestinationBase* UMDFastBindingInstance::SetDestination(UMDFastBindingDestinationBase* InDestination)
{
	if (InDestination == nullptr || BindingDestination == InDestination)
	{
		return BindingDestination;
	}

	Modify();
	UMDFastBindingDestinationBase* Destination = InDestination;
	if (Destination->GetOuter() != this)
	{
		Destination = DuplicateObject<UMDFastBindingDestinationBase>(Destination, this);
	}
	
	if (BindingDestination != nullptr)
	{
		InactiveDestinations.Add(BindingDestination);
		BindingDestination = nullptr;
	}

	InactiveDestinations.Remove(Destination);
	BindingDestination = Destination;

	return BindingDestination;
}

void UMDFastBindingInstance::RemoveDestination(UMDFastBindingDestinationBase* InDestination)
{
	if (BindingDestination == InDestination)
	{
		BindingDestination = (InactiveDestinations.Num() > 0) ? InactiveDestinations.Pop() : nullptr;
	}
	else
	{
		InactiveDestinations.Remove(InDestination);
	}
}

FText UMDFastBindingInstance::GetBindingDisplayName() const
{
	if (!BindingName.IsEmpty())
	{
		return FText::FromString(BindingName);
	}

	return FText::FromString(GetName());
}

void UMDFastBindingInstance::SetBindingDisplayName(const FText& InText)
{
	Modify();
	BindingName = InText.ToString();
}
#endif
