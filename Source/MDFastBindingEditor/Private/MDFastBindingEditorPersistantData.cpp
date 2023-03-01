#include "MDFastBindingEditorPersistantData.h"

bool UMDFastBindingEditorPersistantData::IsNodeBeingWatched(const FGuid& NodeId) const
{
	if (const FMDFastBindingEditorPinWatchList* WatchList = NodePinWatchList.Find(NodeId))
	{
		return WatchList->Pins.Num() > 0;
	}

	return false;
}

bool UMDFastBindingEditorPersistantData::IsPinBeingWatched(const FGuid& NodeId, const FName& PinName) const
{
	const FMDFastBindingEditorPinWatchList* WatchList = NodePinWatchList.Find(NodeId);
	return WatchList != nullptr && WatchList->Pins.Contains(PinName);
}

void UMDFastBindingEditorPersistantData::GatherWatchedPins(const FGuid& NodeId, TArray<FName>& Pins) const
{
	if (const FMDFastBindingEditorPinWatchList* WatchList = NodePinWatchList.Find(NodeId))
	{
		Pins = WatchList->Pins.Array();
	}
}

void UMDFastBindingEditorPersistantData::AddPinToWatchList(const FGuid& NodeId, const FName& PinName)
{
	NodePinWatchList.FindOrAdd(NodeId).Pins.Add(PinName);
	
	SaveConfig();

	OnWatchListChanged.Broadcast();
}

void UMDFastBindingEditorPersistantData::RemovePinFromWatchList(const FGuid& NodeId, const FName& PinName)
{
	if (FMDFastBindingEditorPinWatchList* WatchList = NodePinWatchList.Find(NodeId))
	{
		if (WatchList->Pins.Remove(PinName) > 0)
		{
			SaveConfig();
			
			OnWatchListChanged.Broadcast();
		}
	}
}

void UMDFastBindingEditorPersistantData::RemoveNodeFromWatchList(const FGuid& NodeId)
{
	if (NodePinWatchList.Remove(NodeId) > 0)
	{
		SaveConfig();
			
		OnWatchListChanged.Broadcast();
	}
}
