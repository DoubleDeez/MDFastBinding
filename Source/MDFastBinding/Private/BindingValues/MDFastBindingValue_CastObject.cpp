#include "BindingValues/MDFastBindingValue_CastObject.h"

namespace MDFastBindingValue_CastObject_Private
{
	static const FName ObjectName = TEXT("Object");
}

const FProperty* UMDFastBindingValue_CastObject::GetOutputProperty()
{
#if WITH_EDITOR
	ResultProp = nullptr;
	if (IsValid(ObjectClass))
#else
	if (ResultProp == nullptr && IsValid(ObjectClass))
#endif
	{
		if (ObjectClass->HasAllClassFlags(CLASS_Interface))
		{
			if (FProperty* Prop = GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UMDFastBindingValue_CastObject, ResultInterfaceField)))
			{
				if (FInterfaceProperty* InterfaceProp = CastField<FInterfaceProperty>(Prop))
				{
					// This is how we make our output dynamic
					InterfaceProp->InterfaceClass = ObjectClass;
					ResultProp = InterfaceProp;
				}
			}
		}
		else
		{
			if (FProperty* Prop = GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UMDFastBindingValue_CastObject, ResultObject)))
			{
				if (FObjectPropertyBase* ObjProp = CastField<FObjectPropertyBase>(Prop))
				{
					// This is how we make our output dynamic
					ObjProp->PropertyClass = ObjectClass;
					ResultProp = ObjProp;
				}
			}
		}
	}

	return ResultProp;
}

#if WITH_EDITORONLY_DATA
FText UMDFastBindingValue_CastObject::GetDisplayName()
{
	if (ObjectClass != nullptr)
	{
		return ObjectClass->GetDisplayNameText();
	}

	return Super::GetDisplayName();
}
#endif

TTuple<const FProperty*, void*> UMDFastBindingValue_CastObject::GetValue_Internal(UObject* SourceObject)
{
	bool bDidUpdate = false;
	const TTuple<const FProperty*, void*> ObjectValue = GetBindingItemValue(SourceObject, MDFastBindingValue_CastObject_Private::ObjectName, bDidUpdate);

	if (bDidUpdate && ObjectValue.Value != nullptr && ObjectClass != nullptr)
	{
		ResultObject = nullptr;
		ResultInterface = {};

		if (UObject* Object = *static_cast<UObject**>(ObjectValue.Value))
		{
			if (ObjectClass->HasAllClassFlags(CLASS_Interface))
			{
				if (Object->GetClass()->ImplementsInterface(ObjectClass))
				{
					ResultObject = Object;
					ResultInterface = FScriptInterface(Object, Object->GetInterfaceAddress(ObjectClass));
				}
			}
			else if (Object->IsA(ObjectClass))
			{
				ResultObject = Object;
			}
		}
	}

	if (IsValid(ObjectClass) && ObjectClass->HasAllClassFlags(CLASS_Interface))
	{
		return TTuple<const FProperty*, void*>{ GetOutputProperty(), &ResultInterface };
	}
	else
	{
		return TTuple<const FProperty*, void*>{ GetOutputProperty(), &ResultObject };
	}
}

void UMDFastBindingValue_CastObject::SetupBindingItems()
{
	Super::SetupBindingItems();

	EnsureBindingItemExists(MDFastBindingValue_CastObject_Private::ObjectName
			, GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UMDFastBindingValue_CastObject, ObjectField))
			, NSLOCTEXT("MDFastBindingValue_CastObject", "ObjectToolTip", "The object to cast to the selected class."));
}
