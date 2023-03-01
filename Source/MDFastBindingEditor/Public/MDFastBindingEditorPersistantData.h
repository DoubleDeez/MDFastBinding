#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MDFastBindingEditorPersistantData.generated.h"

USTRUCT()
struct FMDFastBindingEditorPinWatchList
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TSet<FName> Pins;
};

/**
 * 
 */
UCLASS(config = "FastBindingEditor")
class MDFASTBINDINGEDITOR_API UMDFastBindingEditorPersistantData : public UObject
{
	GENERATED_BODY()

public:
	static UMDFastBindingEditorPersistantData& Get() { return *GetMutableDefault<UMDFastBindingEditorPersistantData>(); }
	
	bool IsNodeBeingWatched(const FGuid& NodeId) const;
	bool IsPinBeingWatched(const FGuid& NodeId, const FName& PinName) const;
	void GatherWatchedPins(const FGuid& NodeId, TArray<FName>& Pins) const;
	
	void AddPinToWatchList(const FGuid& NodeId, const FName& PinName);
	void RemovePinFromWatchList(const FGuid& NodeId, const FName& PinName);
	void RemoveNodeFromWatchList(const FGuid& NodeId);

	FSimpleMulticastDelegate OnWatchListChanged;

protected:
	// Map binding node GUID to list of pins being watched
	UPROPERTY(Config)
	TMap<FGuid, FMDFastBindingEditorPinWatchList> NodePinWatchList;
};
