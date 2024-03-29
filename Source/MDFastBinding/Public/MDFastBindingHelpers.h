#pragma once

#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "UObject/WeakFieldPtr.h"
#include "MDFastBindingHelpers.generated.h"

class FProperty;
class UFunction;
class UWidgetBlueprintGeneratedClass;

class MDFASTBINDING_API FMDFastBindingHelpers
{
public:
	static void GetFunctionParamProps(const UFunction* Func, TArray<const FProperty*>& OutParams);
	static void SplitFunctionParamsAndReturnProp(const UFunction* Func, TArray<TWeakFieldPtr<const FProperty>>& OutParams, TWeakFieldPtr<const FProperty>& OutReturnProp);

	static FString PropertyToString(const FProperty& Prop);

	static bool ArePropertyValuesEqual(const FProperty* PropA, const void* ValuePtrA, const FProperty* PropB, const void* ValuePtrB);

	static bool DoesClassHaveSuperClassBindings(UWidgetBlueprintGeneratedClass* Class);
};

UCLASS(Hidden, MinimalAPI)
class UMDFastBindingProperties : public UObject
{
	GENERATED_BODY()

public:
	static const FObjectProperty* GetObjectProperty()
	{
		static const FObjectProperty* Property = CastFieldChecked<FObjectProperty>(StaticClass()->FindPropertyByName(TEXT("ObjectProperty")));
		return Property;
	}

private:
	UPROPERTY(Transient)
	TObjectPtr<UObject> ObjectProperty;
};

