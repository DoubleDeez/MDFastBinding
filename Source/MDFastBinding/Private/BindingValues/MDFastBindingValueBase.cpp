// Fill out your copyright notice in the Description page of Project Settings.


#include "BindingValues/MDFastBindingValueBase.h"

#include "BindingDestinations/MDFastBindingDestinationBase.h"

UMDFastBindingDestinationBase* UMDFastBindingValueBase::GetOuterBindingDestination() const
{
#if !WITH_EDITOR
	if (OuterBindingDestination.IsValid())
	{
		return OuterBindingDestination.Get();
	}
#endif

	UObject* Object = GetOuter();
	while (Object != nullptr)
	{
		if (UMDFastBindingDestinationBase* BindingDest = Cast<UMDFastBindingDestinationBase>(Object))
		{
			OuterBindingDestination = BindingDest;
			return BindingDest;
		}

		Object = Object->GetOuter();
	}

	return nullptr;
}

#if WITH_EDITORONLY_DATA
void UMDFastBindingValueBase::OrphanAllBindingItems(const TSet<UObject*>& OrphanExclusionSet)
{
	for (FMDFastBindingItem& Item : BindingItems)
	{
		if (!OrphanExclusionSet.Contains(Item.Value))
		{
			OrphanBindingItem(Item.Value);
			Item.Value = nullptr;
		}
	}
}

void UMDFastBindingValueBase::OrphanBindingItem(UMDFastBindingValueBase* InValue)
{
	if (UMDFastBindingDestinationBase* BindingDest = GetOuterBindingDestination())
	{
		BindingDest->AddOrphan(InValue);
	}
}
#endif
