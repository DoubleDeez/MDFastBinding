#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "MDFastBindingInstance.generated.h"

class UMDFastBindingDestinationBase;
class UMDFastBindingValueBase;
/**
 * 
 */
UCLASS(DefaultToInstanced, EditInlineNew)
class MDFASTBINDING_API UMDFastBindingInstance : public UObject
{
	GENERATED_BODY()

public:
	void InitializeBinding(UObject* SourceObject);
	void UpdateBinding(UObject* SourceObject);
	void TerminateBinding(UObject* SourceObject);

	UMDFastBindingDestinationBase* GetBindingDestination() const { return BindingDestination; }

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(TArray<FText>& ValidationErrors) override;
#endif
	
#if WITH_EDITORONLY_DATA
	UMDFastBindingValueBase* AddOrphan(UMDFastBindingValueBase* InValue);
	void RemoveOrphan(UMDFastBindingValueBase* InValue);

	UMDFastBindingDestinationBase* SetDestination(TSubclassOf<UMDFastBindingDestinationBase> InClass);
	UMDFastBindingDestinationBase* SetDestination(UMDFastBindingDestinationBase* InDestination);
	void RemoveDestination(UMDFastBindingDestinationBase* InDestination);

	FText GetBindingDisplayName() const;

	void SetBindingDisplayName(const FText& InText);
	
	UPROPERTY(Instanced)
	TArray<UMDFastBindingValueBase*> OrphanedValues;
	
	UPROPERTY(Instanced)
	TArray<UMDFastBindingDestinationBase*> InactiveDestinations;

	UPROPERTY()
	FString BindingName;
#endif

protected:
	UPROPERTY(Instanced)
	UMDFastBindingDestinationBase* BindingDestination = nullptr;
};
