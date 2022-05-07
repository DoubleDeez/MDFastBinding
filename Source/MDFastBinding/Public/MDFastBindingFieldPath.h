#pragma once

#include "CoreMinimal.h"
#include "MDFastBindingMemberReference.h"
#include "MDFastBindingFieldPath.generated.h"

DECLARE_DELEGATE_RetVal_OneParam(UObject*, FMDGetFieldPathOwner, UObject*);
DECLARE_DELEGATE_RetVal(UClass*, FMDGetFieldPathOwnerClass);

/**
 * 
 */
USTRUCT()
struct MDFASTBINDING_API FMDFastBindingFieldPath
{
	GENERATED_BODY()

public:
	~FMDFastBindingFieldPath();
	
	bool BuildPath();
	const TArray<FFieldVariant>& GetFieldPath();

	// Returns a tuple containing the leaf property in the path (or return value property if a function) and a pointer to the value
	TTuple<const FProperty*, void*> ResolvePath(UObject* SourceObject);

	const FProperty* GetLeafProperty();
	bool IsLeafFunction();

	bool IsPropertyValidForPath(const FProperty& Prop) const;
	static bool IsFunctionValidForPath(const UFunction& Func);
	
	UClass* GetPathOwnerClass() const;

	FString ToString() const;

#if WITH_EDITOR
	void OnVariableRenamed(UClass* VariableClass, const FName& OldVariableName, const FName& NewVariableName);
#endif
	
	FMDGetFieldPathOwner OwnerGetter;
	FMDGetFieldPathOwnerClass OwnerClassGetter;

	// Set to true if you're going to be setting the value of the property
	bool bOnlyAllowBlueprintReadWriteProperties = false;

	UPROPERTY(meta = (DeprecatedProperty))
	TArray<FName> FieldPath;

	UPROPERTY(EditAnywhere, Category = "Bindings")
	TArray<FMDFastBindingMemberReference> FieldPathMembers;

private:
	void* GetPathOwner(UObject* SourceObject) const;
	void InitFunctionMemory(const UFunction* Func);
	void CleanupFunctionMemory();

	void FixupFieldPath();
	
	TArray<FFieldVariant> CachedPath;
	TMap<TWeakObjectPtr<const UFunction>, void*> FunctionMemory;

	// Cached to be able to provide a UObject**
	mutable UObject* CachedOwnerObject = nullptr;
};
