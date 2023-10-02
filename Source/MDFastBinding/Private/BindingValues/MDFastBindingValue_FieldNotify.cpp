#include "BindingValues/MDFastBindingValue_FieldNotify.h"

#include "Runtime/Launch/Resources/Version.h"
#if ENGINE_MAJOR_VERSION > 5 || ENGINE_MINOR_VERSION >= 3
#include "INotifyFieldValueChanged.h"
#else
#include "FieldNotification/IFieldValueChanged.h"
#endif

UMDFastBindingValue_FieldNotify::UMDFastBindingValue_FieldNotify()
{
	UpdateType = EMDFastBindingUpdateType::EventBased;
	PropertyPath.bAllowGetterFunctions = true;
	PropertyPath.bAllowSubProperties = false;
}

void UMDFastBindingValue_FieldNotify::PostInitProperties()
{
	PropertyPath.FieldFilter.BindUObject(this, &UMDFastBindingValue_FieldNotify::IsValidFieldNotify);

	Super::PostInitProperties();
}

#if WITH_EDITOR
EDataValidationResult UMDFastBindingValue_FieldNotify::IsDataValid(TArray<FText>& ValidationErrors)
{
	EDataValidationResult Result = Super::IsDataValid(ValidationErrors);

	if (!GetFieldId().IsValid())
	{
		Result = EDataValidationResult::Invalid;
		ValidationErrors.Add(NSLOCTEXT("MDFastBindingValue_FieldNotify", "FieldNotFoundError", "Could not find a corresponding FieldNotify property"));
	}

	return Result;
}
#endif

void UMDFastBindingValue_FieldNotify::InitializeValue_Internal(UObject* SourceObject)
{
	Super::InitializeValue_Internal(SourceObject);

	BindFieldNotify(SourceObject);
}

TTuple<const FProperty*, void*> UMDFastBindingValue_FieldNotify::GetValue_Internal(UObject* SourceObject)
{
	// If the owner has changed, we need to rebind to the delegate
	if (Cast<INotifyFieldValueChanged>(GetPropertyOwner(SourceObject)) != BoundInterface.Get())
	{
		BindFieldNotify(SourceObject);
	}

	return Super::GetValue_Internal(SourceObject);
}

void UMDFastBindingValue_FieldNotify::TerminateValue_Internal(UObject* SourceObject)
{
	Super::TerminateValue_Internal(SourceObject);

	UnbindFieldNotify();
}

const FProperty* UMDFastBindingValue_FieldNotify::GetPathRootProperty() const
{
	return GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UMDFastBindingValue_FieldNotify, FieldNotifyInterface));
}

void UMDFastBindingValue_FieldNotify::OnFieldNotifyValueChanged(UObject* Object, UE::FieldNotification::FFieldId FieldId)
{
	MarkObjectDirty();
}

bool UMDFastBindingValue_FieldNotify::IsValidFieldNotify(const FFieldVariant& Field) const
{
	if (Field.IsValid())
	{
		if (const UClass* Class = Field.GetOwnerClass())
		{
			if (const TScriptInterface<INotifyFieldValueChanged> FieldNotify = Class->GetDefaultObject())
			{
				return FieldNotify->GetFieldNotificationDescriptor().GetField(Class, Field.GetFName()).IsValid();
			}
		}
	}

	return false;
}

void UMDFastBindingValue_FieldNotify::BindFieldNotify(UObject* SourceObject)
{
	UnbindFieldNotify();

	const UE::FieldNotification::FFieldId FieldId = GetFieldId(SourceObject);

	if (FieldId.IsValid())
	{
		if (UObject* PropertyOwner = GetPropertyOwner(SourceObject))
		{
			if (INotifyFieldValueChanged* FieldNotify = Cast<INotifyFieldValueChanged>(PropertyOwner))
			{
				BoundInterface = FieldNotify;
				BoundFieldId = FieldId;
				FieldNotifyHandle = FieldNotify->AddFieldValueChangedDelegate(FieldId
					, INotifyFieldValueChanged::FFieldValueChangedDelegate::CreateUObject(this, &UMDFastBindingValue_FieldNotify::OnFieldNotifyValueChanged));
			}
		}
	}
}

void UMDFastBindingValue_FieldNotify::UnbindFieldNotify()
{
	if (BoundFieldId.IsValid())
	{
		if (INotifyFieldValueChanged* FieldNotify = BoundInterface.Get())
		{
			FieldNotify->RemoveFieldValueChangedDelegate(BoundFieldId, FieldNotifyHandle);
		}
	}

	BoundInterface.Reset();
	BoundFieldId = {};
}

UE::FieldNotification::FFieldId UMDFastBindingValue_FieldNotify::GetFieldId()
{
	if (const FFieldVariant Field = GetLeafField())
	{
		if (const UClass* OwnerClass = Cast<UClass>(GetPropertyOwnerStruct()))
		{
			if (const INotifyFieldValueChanged* FieldNotify = Cast<INotifyFieldValueChanged>(OwnerClass->GetDefaultObject()))
			{
				return FieldNotify->GetFieldNotificationDescriptor().GetField(OwnerClass, Field.GetFName());
			}
		}
	}

	return {};
}

UE::FieldNotification::FFieldId UMDFastBindingValue_FieldNotify::GetFieldId(UObject* SourceObject)
{
	if (const FFieldVariant Field = GetLeafField())
	{
		if (UObject* PropertyOwner = GetPropertyOwner(SourceObject))
		{
			if (const INotifyFieldValueChanged* FieldNotify = Cast<INotifyFieldValueChanged>(PropertyOwner))
			{
				return FieldNotify->GetFieldNotificationDescriptor().GetField(PropertyOwner->GetClass(), Field.GetFName());
			}
		}
	}

	return {};
}
