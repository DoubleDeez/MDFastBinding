#pragma once

#include "MDFastBindingValue_Property.h"
#include "FieldNotification/FieldId.h"
#include "UObject/WeakInterfacePtr.h"
#include "MDFastBindingValue_FieldNotify.generated.h"

class INotifyFieldValueChanged;

/**
 * Retrieve the value of a FieldNotify marked property any time it changes
 */
UCLASS(meta = (DisplayName = "Field Notify"))
class MDFASTBINDING_API UMDFastBindingValue_FieldNotify : public UMDFastBindingValue_Property
{
	GENERATED_BODY()

public:
	UMDFastBindingValue_FieldNotify();

	virtual void PostInitProperties() override;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(TArray<FText>& ValidationErrors) override;
#endif

protected:
	virtual void InitializeValue_Internal(UObject* SourceObject) override;
	virtual TTuple<const FProperty*, void*> GetValue_Internal(UObject* SourceObject) override;
	virtual void TerminateValue_Internal(UObject* SourceObject) override;

	virtual const FProperty* GetPathRootProperty() const override;

	virtual void OnFieldNotifyValueChanged(UObject* Object, UE::FieldNotification::FFieldId FieldId);

	bool IsValidFieldNotify(const FFieldVariant& Field) const;

	void BindFieldNotify(UObject* SourceObject);
	void UnbindFieldNotify();

	UE::FieldNotification::FFieldId GetFieldId();
	UE::FieldNotification::FFieldId GetFieldId(UObject* SourceObject);

	UPROPERTY(Transient)
	TScriptInterface<INotifyFieldValueChanged> FieldNotifyInterface;

private:
	FDelegateHandle FieldNotifyHandle;
	TWeakInterfacePtr<INotifyFieldValueChanged> BoundInterface;
	UE::FieldNotification::FFieldId BoundFieldId;
};
