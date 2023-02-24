#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MDFastBindingInstance.generated.h"

class UMDFastBindingContainer;
class UMDFastBindingDestinationBase;
class UMDFastBindingObject;
class UMDFastBindingValueBase;

/**
 * 
 */
UCLASS(DefaultToInstanced, EditInlineNew)
class MDFASTBINDING_API UMDFastBindingInstance : public UObject
{
	GENERATED_BODY()

public:
	UClass* GetBindingOuterClass() const;

	UMDFastBindingContainer* GetBindingContainer() const;
	
	void InitializeBinding(UObject* SourceObject);
	void UpdateBinding(UObject* SourceObject);
	void TerminateBinding(UObject* SourceObject);

	UMDFastBindingDestinationBase* GetBindingDestination() const { return BindingDestination; }

	bool ShouldBindingTick() const;

	void MarkBindingDirty();

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(TArray<FText>& ValidationErrors) override;

	void OnVariableRenamed(UClass* VariableClass, const FName& OldVariableName, const FName& NewVariableName);

	// Returns false if any nodes use the `Always` update type
	bool IsBindingPerformant() const;
#endif
	
#if WITH_EDITORONLY_DATA
	UMDFastBindingValueBase* AddOrphan(UMDFastBindingValueBase* InValue);
	void RemoveOrphan(UMDFastBindingValueBase* InValue);

	UMDFastBindingDestinationBase* SetDestination(TSubclassOf<UMDFastBindingDestinationBase> InClass);
	UMDFastBindingDestinationBase* SetDestination(UMDFastBindingDestinationBase* InDestination);
	void RemoveDestination(UMDFastBindingDestinationBase* InDestination);

	FText GetBindingDisplayName() const;

	void SetBindingDisplayName(const FText& InText);

	UMDFastBindingObject* FindBindingObjectWithGUID(const FGuid& Guid) const;
	
	UPROPERTY(Instanced)
	TArray<UMDFastBindingValueBase*> OrphanedValues;
	
	UPROPERTY(Instanced)
	TArray<UMDFastBindingDestinationBase*> InactiveDestinations;

	UPROPERTY()
	FString BindingName;

private:
	UPROPERTY(Transient)
	mutable TMap<FGuid, TWeakObjectPtr<UMDFastBindingObject>> GuidToBindingObjectMap;
public:
#endif

protected:
	UPROPERTY(Instanced)
	UMDFastBindingDestinationBase* BindingDestination = nullptr;
};
