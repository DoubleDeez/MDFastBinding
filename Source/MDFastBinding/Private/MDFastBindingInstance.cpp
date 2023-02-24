#include "MDFastBindingInstance.h"

#include "MDFastBindingContainer.h"
#include "BindingDestinations/MDFastBindingDestinationBase.h"
#include "BindingValues/MDFastBindingValueBase.h"

UClass* UMDFastBindingInstance::GetBindingOuterClass() const
{
	if (const UMDFastBindingContainer* BindingContainer = GetBindingContainer())
	{
		if (BindingContainer != nullptr)
		{
			return BindingContainer->GetOuter()->GetClass();
		}
	}

	return nullptr;
}

UMDFastBindingContainer* UMDFastBindingInstance::GetBindingContainer() const
{
	UObject* Object = GetOuter();
	while (Object != nullptr)
	{
		if (UMDFastBindingContainer* BindingContainer = Cast<UMDFastBindingContainer>(Object))
		{
			return BindingContainer;
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

bool UMDFastBindingInstance::ShouldBindingTick() const
{
	if (BindingDestination != nullptr)
	{
		return BindingDestination->DoesObjectRequireTick();
	}
	
	return false;
}

void UMDFastBindingInstance::MarkBindingDirty()
{
	if (UMDFastBindingContainer* BindingContainer = GetBindingContainer())
	{
		constexpr bool bShouldTick = true;
		BindingContainer->SetBindingTickPolicy(this, bShouldTick);
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

bool UMDFastBindingInstance::IsBindingPerformant() const
{
	if (BindingDestination != nullptr)
	{
		TFunction<bool(UMDFastBindingObject*)> IsBindingObjectPerformant;

		IsBindingObjectPerformant = [&IsBindingObjectPerformant](UMDFastBindingObject* BindingObject) -> bool
		{
			if (BindingObject == nullptr)
			{
				return true;
			}
			
			const EMDFastBindingUpdateType UpdateType = BindingObject->GetUpdateType();
			if (UpdateType == EMDFastBindingUpdateType::Once)
			{
				// Once doesn't care about their binding items when deciding update frequency
				return true;
			}

			if (UpdateType == EMDFastBindingUpdateType::Always)
			{
				return false;
			}

			// IfUpdatesNeeded and EventBased are performant if their binding items are performant
			for (const FMDFastBindingItem& BindingItem : BindingObject->GetBindingItems())
			{
				if (!IsBindingObjectPerformant(BindingItem.Value))
				{
					return false;
				}
			}

			return true;
		};
		
		return IsBindingObjectPerformant(BindingDestination);
	}

	return false;
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

UMDFastBindingObject* UMDFastBindingInstance::FindBindingObjectWithGUID(const FGuid& Guid) const
{
	if (GuidToBindingObjectMap.Contains(Guid))
	{
		if (UMDFastBindingObject* Object = GuidToBindingObjectMap[Guid].Get())
		{
			return Object;
		}
	}

	auto CheckObject = [&](UMDFastBindingObject* Object)
	{
		if (Object != nullptr && Object->BindingObjectIdentifier == Guid)
		{
			GuidToBindingObjectMap.Add(Guid, Object);
			return true;
		}

		return false;
	};
	
	// Traverse the tree of nodes connected to BindingDestination
	if (BindingDestination != nullptr)
	{
		TFunction<UMDFastBindingObject*(UMDFastBindingObject*)> CheckBindingNodeTree;

		CheckBindingNodeTree = [&](UMDFastBindingObject* BindingObject) -> UMDFastBindingObject*
		{
			if (BindingObject == nullptr)
			{
				return nullptr;
			}

			if (CheckObject(BindingObject))
			{
				return BindingObject;
			}

			for (const FMDFastBindingItem& BindingItem : BindingObject->GetBindingItems())
			{
				if (UMDFastBindingObject* FoundObject = CheckBindingNodeTree(BindingItem.Value))
				{
					return FoundObject;
				}
			}

			return nullptr;
		};
		
		if (UMDFastBindingObject* FoundObject = CheckBindingNodeTree(BindingDestination))
		{
			return FoundObject;
		}
	}
	
	// Check if the object is an orphan / inactive destination 
	{
		for (UMDFastBindingValueBase* Orphan : OrphanedValues)
		{
			if (CheckObject(Orphan))
			{
				return Orphan;
			}
		}

		for (UMDFastBindingDestinationBase* InactiveDestination : InactiveDestinations)
		{
			if (CheckObject(InactiveDestination))
			{
				return InactiveDestination;
			}
		}
	}

	return nullptr;
}
#endif
