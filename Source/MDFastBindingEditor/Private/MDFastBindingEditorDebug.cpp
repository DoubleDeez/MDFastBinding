#include "MDFastBindingEditorDebug.h"

#include "BlueprintEditor.h"
#include "EdGraphSchema_K2.h"
#include "MDFastBindingObject.h"
#include "Kismet2/KismetDebugUtilities.h"


bool FMDFastBindingDebugLineItem::HasChildren() const
{
	if (!CachedChildren.IsSet())
	{
		UpdateCachedChildren();
	}
		
	return CachedChildren.GetValue().Num() > 0;
}

TSharedRef<SWidget> FMDFastBindingDebugLineItem::GetNameIcon()
{
	if (const FProperty* ItemProperty = GetItemProperty())
	{
		FSlateColor BaseColor;
		FSlateColor UnusedColor;
		FSlateBrush const* UnusedIcon = nullptr;
		const FSlateBrush* IconBrush = FBlueprintEditor::GetVarIconAndColorFromProperty(
			ItemProperty,
			BaseColor,
			UnusedIcon,
			UnusedColor
		);

		return SNew(SImage)
			.Image(IconBrush)
			.ColorAndOpacity(BaseColor)
			.ToolTipText(UEdGraphSchema_K2::TypeToText(ItemProperty));
	}

	return FDebugLineItem::GetNameIcon();
}

TSharedRef<SWidget> FMDFastBindingDebugLineItem::GenerateValueWidget(TSharedPtr<FString> InSearchString)
{
	return SNew(STextBlock)
		.Text(this, &FMDFastBindingDebugLineItem::GetDisplayValue)
		.ToolTipText(this, &FMDFastBindingDebugLineItem::GetDisplayValue);
}

void FMDFastBindingDebugLineItem::GatherChildrenBase(TArray<FDebugTreeItemPtr>& OutChildren, const FString& InSearchString, bool bRespectSearch)
{
	if (!CachedChildren.IsSet())
	{
		UpdateCachedChildren();
	}

	OutChildren.Append(CachedChildren.GetValue());
}

const FProperty* FMDFastBindingDebugLineItem::GetItemProperty() const
{
	return GetPropertyInstance().Key;
}

FText FMDFastBindingDebugLineItem::GetDisplayValue() const
{
	const TTuple<const FProperty*, void*> PropertyInstance = GetPropertyInstance();
	if (PropertyInstance.Key != nullptr && PropertyInstance.Value != nullptr)
	{
		if (PropertyInstance.Key->IsA<FObjectProperty>() || PropertyInstance.Key->IsA<FInterfaceProperty>())
		{
			const UObject* Object = nullptr;
			if (const FObjectPropertyBase* ObjectPropertyBase = CastField<FObjectPropertyBase>(PropertyInstance.Key))
			{
				Object = ObjectPropertyBase->GetObjectPropertyValue(PropertyInstance.Value);
			}
			else if (PropertyInstance.Key->IsA<FInterfaceProperty>())
			{
				const FScriptInterface* InterfaceData = static_cast<const FScriptInterface*>(PropertyInstance.Value);
				Object = InterfaceData->GetObject();
			}

			if (Object != nullptr)
			{
				return FText::FromString(FString::Printf(TEXT("%s (Class: %s)"), *Object->GetName(), *Object->GetClass()->GetName()));
			}
			else
			{
				return INVTEXT("[None]");
			}
		}
		else if (PropertyInstance.Key->IsA<FStructProperty>())
		{
			if (!CachedChildren.IsSet())
			{
				UpdateCachedChildren();
			}
		
			return FText::Format(INVTEXT("{0} {0}|plural(one=member,other=members)"), FText::AsNumber(CachedChildren.GetValue().Num()));
		}
		else
		{
			TSharedPtr<FPropertyInstanceInfo> DebugInfo;
			FKismetDebugUtilities::GetDebugInfoInternal(DebugInfo, PropertyInstance.Key, PropertyInstance.Value);
			return DebugInfo->Value;
		}
	}
	
	return INVTEXT("[No Value]");
}

void FMDFastBindingDebugLineItem::UpdateCachedChildren() const
{
	CachedChildren = TArray<FDebugTreeItemPtr>();
	
	if (const FProperty* ItemProperty = GetItemProperty())
	{
		const TTuple<const FProperty*, void*> PropertyInstance = GetPropertyInstance();
		const UStruct* PropertyStruct = nullptr;
		void* PropertyValue = nullptr;
		if (const FObjectPropertyBase* ObjectProp = CastField<FObjectPropertyBase>(ItemProperty))
		{
			PropertyStruct = ObjectProp->PropertyClass;
			if (PropertyInstance.Value != nullptr)
			{
				PropertyValue = ObjectProp->GetObjectPropertyValue(PropertyInstance.Value);
			}
		}
		else if (const FStructProperty* StructProp = CastField<FStructProperty>(ItemProperty))
		{
			PropertyStruct = StructProp->Struct;
			PropertyValue = PropertyInstance.Value;
		}
		else if (ItemProperty->IsA<FInterfaceProperty>())
		{
			// Interface property won't have the class so we need the actual value to get it
			if (const FScriptInterface* InterfaceData = static_cast<const FScriptInterface*>(PropertyInstance.Value))
			{
				if (UObject* Object = InterfaceData->GetObject())
				{
					PropertyStruct = Object->GetClass();
					PropertyValue = Object;
				}
			}
		}

		if (PropertyStruct != nullptr)
		{
			for (TFieldIterator<FProperty> It(PropertyStruct); It; ++It)
			{
				if (const FProperty* ChildProp = *It)
				{
					void* ChildValuePtr = PropertyValue != nullptr ? ChildProp->ContainerPtrToValuePtr<void>(PropertyValue) : nullptr;
					CachedChildren.GetValue().Add(MakeShared<FMDFastBindingPropertyDebugLineItem>(ChildProp, ChildValuePtr));
				}
			}
		}
		else if (const FArrayProperty* ArrayProp = CastField<FArrayProperty>(ItemProperty))
		{
			if (PropertyInstance.Value != nullptr)
			{
				FScriptArrayHelper Helper = FScriptArrayHelper(ArrayProp, PropertyInstance.Value);
				for (int32 i = 0; i < Helper.Num(); ++i)
				{
					void* ChildValuePtr = Helper.GetRawPtr(i);
					const FText ElementDisplayName = FText::Format(INVTEXT("[{0}]"), FText::AsNumber(i));
					CachedChildren.GetValue().Add(MakeShared<FMDFastBindingPropertyDebugLineItem>(ArrayProp->Inner, ChildValuePtr, ElementDisplayName));
				}
			}
		}
		else if (const FSetProperty* SetProp = CastField<FSetProperty>(ItemProperty))
		{
			if (PropertyInstance.Value != nullptr)
			{
				FScriptSetHelper Helper = FScriptSetHelper(SetProp, PropertyInstance.Value);
				for (int32 i = 0; i < Helper.Num(); ++i)
				{
					void* ChildValuePtr = Helper.GetElementPtr(i);
					const FText ElementDisplayName = FText::Format(INVTEXT("[{0}]"), FText::AsNumber(i));
					CachedChildren.GetValue().Add(MakeShared<FMDFastBindingPropertyDebugLineItem>(SetProp->ElementProp, ChildValuePtr, ElementDisplayName));
				}
			}
		}
		else if (const FMapProperty* MapProp = CastField<FMapProperty>(ItemProperty))
		{
			if (PropertyInstance.Value != nullptr)
			{
				FScriptMapHelper Helper = FScriptMapHelper(MapProp, PropertyInstance.Value);
				for (int32 i = 0; i < Helper.Num(); ++i)
				{
					// Key
					void* ChildKeyPtr = Helper.GetKeyPtr(i);
					const FText KeyDisplayName = FText::Format(INVTEXT("Key[{0}]"), FText::AsNumber(i));
					CachedChildren.GetValue().Add(MakeShared<FMDFastBindingPropertyDebugLineItem>(MapProp->KeyProp, ChildKeyPtr, KeyDisplayName));

					// Value
					void* ChildValuePtr = Helper.GetValuePtr(i);
					const FText ValueDisplayName = FText::Format(INVTEXT("Value[{0}]"), FText::AsNumber(i));
					CachedChildren.GetValue().Add(MakeShared<FMDFastBindingPropertyDebugLineItem>(MapProp->ValueProp, ChildValuePtr, KeyDisplayName));
				}
			}
		}
	}
}

const FMDFastBindingItem* FMDFastBindingItemDebugLineItem::GetBindingItem() const
{
	if (UMDFastBindingObject* DebugObject = DebugObjectPtr.Get())
	{
		return DebugObject->FindBindingItem(ItemName);
	}

	return nullptr;
}

const FProperty* FMDFastBindingItemDebugLineItem::GetItemProperty() const
{
	if (const FMDFastBindingItem* BindingItem = GetBindingItem())
	{
		// Try to get the runtime value property if there is one
		if (const FProperty* RuntimeProperty = BindingItem->GetCachedValue().Key)
		{
			return RuntimeProperty;
		}

		// Fallback to the design-time property
		return BindingItem->ResolveOutputProperty();
	}

	return nullptr;
}

TTuple<const FProperty*, void*> FMDFastBindingItemDebugLineItem::GetPropertyInstance() const
{
	if (const FMDFastBindingItem* BindingItem = GetBindingItem())
	{
		return BindingItem->GetCachedValue();
	}

	return {};
}


TTuple<const FProperty*, void*> FMDFastBindingPropertyDebugLineItem::GetPropertyInstance() const
{
	return TTuple<const FProperty*, void*>{ PropertyPtr.Get(), ValuePtr };
}

void SMDFastBindingPinValueInspector::SetReferences(UEdGraphPin* Pin, UMDFastBindingObject* DebugObject)
{
	PinName = Pin->PinName;
	DebugObjectPtr = DebugObject;
}

bool SMDFastBindingPinValueInspector::Matches(UEdGraphPin* Pin, UMDFastBindingObject* DebugObject) const
{
	return Pin != nullptr && Pin->PinName == PinName && DebugObjectPtr == DebugObject;
}

void SMDFastBindingPinValueInspector::PopulateTreeView()
{
	if (UMDFastBindingObject* DebugObject = DebugObjectPtr.Get())
	{
		const TSharedPtr<FMDFastBindingItemDebugLineItem> Item = MakeShared<FMDFastBindingItemDebugLineItem>(DebugObject, PinName);
		AddTreeItemUnique(Item);
	}
}
