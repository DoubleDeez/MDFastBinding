#include "BindingValues/MDFastBindingValue_CastObject.h"

namespace MDFastBindingValue_CastObject_Private
{
	static const FName ObjectName = TEXT("Object");
}

const FProperty* UMDFastBindingValue_CastObject::GetOutputProperty()
{
#if !WITH_EDITOR
	if (ObjectProp == nullptr)
#endif
	{
		if (FProperty* Prop = GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UMDFastBindingValue_CastObject, ResultObject)))
		{
			if (FObjectPropertyBase* ObjProp = CastField<FObjectPropertyBase>(Prop))
			{
				// This is how we make our output dynamic
				ObjProp->PropertyClass = ObjectClass;
				ObjectProp = ObjProp;
			}
		}
	}
	
	return ObjectProp;
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

TTuple<const FProperty*, void*> UMDFastBindingValue_CastObject::GetValue_Internal(UObject* SourceObject)
{
	bool bDidUpdate = false;
	const TTuple<const FProperty*, void*> ObjectValue = GetBindingItemValue(SourceObject, MDFastBindingValue_CastObject_Private::ObjectName, bDidUpdate);
	
	if (bDidUpdate && ObjectValue.Value != nullptr && ObjectClass != nullptr)
	{
		ResultObject = nullptr;
		
		if (UObject* Object = *static_cast<UObject**>(ObjectValue.Value))
		{
			if (Object->IsA(ObjectClass))
			{
				ResultObject = Object;
			}
		}
	}

	return { GetOutputProperty(), &ResultObject };
}

void UMDFastBindingValue_CastObject::SetupBindingItems()
{
	Super::SetupBindingItems();

	EnsureBindingItemExists(MDFastBindingValue_CastObject_Private::ObjectName
			, GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UMDFastBindingValue_CastObject, ResultObject))
			, NSLOCTEXT("MDFastBindingValue_CastObject", "ObjectToolTip", "The object to cast to the selected class."));
}
#endif
