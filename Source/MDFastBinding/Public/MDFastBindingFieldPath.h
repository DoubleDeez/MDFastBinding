#pragma once

#include "MDFastBindingMemberReference.h"
#include "UObject/UnrealType.h"
#include "UObject/WeakFieldPtr.h"

#include "MDFastBindingFieldPath.generated.h"

DECLARE_DELEGATE_RetVal_OneParam(UObject*, FMDGetFieldPathOwner, UObject*);
DECLARE_DELEGATE_RetVal(UStruct*, FMDGetFieldPathOwnerStruct);
DECLARE_DELEGATE_RetVal_OneParam(bool, FMDFilterFieldPathField, const FFieldVariant&);

// Wraps FFieldVariant to weakly hold the field
struct FMDFastBindingWeakFieldVariant
{
public:
	FMDFastBindingWeakFieldVariant(const FField* InField)
		: FieldVariant(InField)
		, FieldCanary(InField ? InField->GetOwnerUObject() : nullptr)
	{
	}

	FMDFastBindingWeakFieldVariant(UField* InField)
		: FieldVariant(InField)
		, FieldCanary(InField)
	{
	}

	bool IsFieldValid() const
	{
		return FieldCanary.IsValid();
	}

	FField* ToField() const
	{
		return IsFieldValid() ? FieldVariant.ToField() : nullptr;
	}

	UObject* ToUObject() const
	{
		return IsFieldValid() ? FieldVariant.ToUObject() : nullptr;
	}

	const FFieldVariant& GetFieldVariant() const
	{
		static FFieldVariant InvalidField = {};
		return IsFieldValid() ? FieldVariant : InvalidField;
	}

private:
	FFieldVariant FieldVariant;

	// Either the field itself or the FField owner, if it's invalid then our field is invalid
	TWeakObjectPtr<UObject> FieldCanary;
};

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
	TArray<FFieldVariant> GetFieldPath();
	const TArray<FMDFastBindingWeakFieldVariant>& GetWeakFieldPath();

	// Returns a tuple containing the leaf property in the path (or return value property if a function) and a pointer to the value,
	// with an optional out param to retrieve the container that holds the leaf property
	TTuple<const FProperty*, void*> ResolvePath(UObject* SourceObject);
	TTuple<const FProperty*, void*> ResolvePath(UObject* SourceObject, void*& OutContainer);
	TTuple<const FProperty*, void*> ResolvePathFromRootObject(UObject* RootObject, void*& OutContainer);

	FFieldVariant GetLeafField();
	UE::FieldNotification::FFieldId GetLeafFieldId();
	const FProperty* GetLeafProperty();
	bool IsLeafFunction();

	bool IsPropertyValidForPath(const FProperty& Prop) const;
	bool IsFunctionValidForPath(const UFunction& Func) const;

	UStruct* GetPathOwnerStruct() const;

	FString ToString() const;

#if WITH_EDITOR
	void OnVariableRenamed(UClass* VariableClass, const FName& OldVariableName, const FName& NewVariableName);
#endif

	FMDGetFieldPathOwner OwnerGetter;
	FMDGetFieldPathOwnerStruct OwnerStructGetter;
	FMDFilterFieldPathField FieldFilter;

	// Set to true if you're going to be setting the value of the property
	bool bOnlyAllowBlueprintReadWriteProperties = false;

	// Set to false if you only want to be able to select top-level properties
	bool bAllowSubProperties = true;

	// Set to false if you only want to allow properties in your field path
	bool bAllowGetterFunctions = true;

	UPROPERTY(meta = (DeprecatedProperty))
	TArray<FName> FieldPath;

	UPROPERTY(EditAnywhere, Category = "Bindings")
	TArray<FMDFastBindingMemberReference> FieldPathMembers;

private:
	void* InitAndGetFunctionMemory(const UFunction* Func);
	void CleanupFunctionMemory();

	void* InitAndGetPropertyMemory(const FProperty* Property);
	void CleanupPropertyMemory();

	void FixupFieldPath();

#if WITH_EDITORONLY_DATA
	TOptional<uint64> LastFrameUpdatedPath;
#endif

	TArray<FMDFastBindingWeakFieldVariant> CachedPath;
	TMap<TWeakObjectPtr<const UFunction>, void*> FunctionMemory;
	TMap<TWeakFieldPtr<FProperty>, void*> PropertyMemory;
};
