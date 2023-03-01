#include "MDFastBindingEditorDebug.h"

#include "BlueprintEditor.h"
#include "EdGraphSchema_K2.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "MDFastBindingEditorPersistantData.h"
#include "MDFastBindingInstance.h"
#include "MDFastBindingObject.h"
#include "Kismet2/KismetDebugUtilities.h"


bool FMDFastBindingDebugLineItemBase::HasChildren() const
{
	if (!CachedChildren.IsSet())
	{
		UpdateCachedChildren();
	}
		
	return CachedChildren.GetValue().Num() > 0;
}

void FMDFastBindingDebugLineItemBase::GatherChildrenBase(TArray<FDebugTreeItemPtr>& OutChildren, const FString& InSearchString, bool bRespectSearch)
{
	if (!CachedChildren.IsSet())
	{
		UpdateCachedChildren();
	}

	OutChildren.Append(CachedChildren.GetValue());
}

void FMDFastBindingWatchedObjectNodeLineItem::RefreshWatchedObject(UMDFastBindingObject* Object)
{
	bIsObjectDirty = WatchedObjectPtr.Get() != Object;
	WatchedObjectPtr = Object;
	UpdateCachedChildren();
}

void FMDFastBindingWatchedObjectNodeLineItem::UpdateCachedChildren() const
{
	CachedChildren = TArray<FDebugTreeItemPtr>();
	
	if (UMDFastBindingObject* WatchedObject = WatchedObjectPtr.Get())
	{
		TArray<FName> WatchedPins;
		UMDFastBindingEditorPersistantData::Get().GatherWatchedPins(WatchedObject->BindingObjectIdentifier, WatchedPins);

		// Remove unwatched pins
		for (auto It = CachedPins.CreateIterator(); It; ++It)
		{
			if (!WatchedPins.Contains(It.Key()))
			{
				It.RemoveCurrent();
			}
		}

		// Add newly watched pins
		for (const FName& Pin : WatchedPins)
		{
			if (!CachedPins.Contains(Pin))
			{
				const TSharedPtr<FMDFastBindingItemDebugLineItem> Item = MakeShared<FMDFastBindingItemDebugLineItem>(WatchedObject, Pin);
				CachedPins.Add(Pin, Item);
			}
			else if (bIsObjectDirty)
			{
				const TSharedPtr<FMDFastBindingItemDebugLineItem> Item = StaticCastSharedPtr<FMDFastBindingItemDebugLineItem>(CachedPins[Pin]);
				Item->RefreshDebugObject(WatchedObject);
			}
		}
	}

	bIsObjectDirty = false;
	CachedPins.GenerateValueArray(CachedChildren.GetValue());
}

UObject* FMDFastBindingWatchedObjectNodeLineItem::GetParentObject()
{
	return WatchedObjectPtr.Get();
}

void FMDFastBindingWatchedObjectNodeLineItem::ExtendContextMenu(FMenuBuilder& MenuBuilder, bool bInDebuggerTab)
{
	FMDFastBindingDebugLineItemBase::ExtendContextMenu(MenuBuilder, bInDebuggerTab);

	MenuBuilder.AddMenuEntry(
		INVTEXT("Remove Watch"),
		INVTEXT("Remove this object from the Watch window"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateWeakLambda(WatchedObjectPtr.Get(), [Obj = WatchedObjectPtr]()
			{
				if (const UMDFastBindingObject* WatchedObject = Obj.Get())
				{
					UMDFastBindingEditorPersistantData::Get().RemoveNodeFromWatchList(WatchedObject->BindingObjectIdentifier);
				}
			}),
			FCanExecuteAction()
		)
	);
}

FText FMDFastBindingWatchedObjectNodeLineItem::GetDisplayName() const
{
	if (UMDFastBindingObject* WatchedObject = WatchedObjectPtr.Get())
	{
		return WatchedObject->DevName.IsEmptyOrWhitespace() ? WatchedObject->GetClass()->GetDisplayNameText() : WatchedObject->DevName;
	}
	
	return INVTEXT("[Invalid]");
}

FText FMDFastBindingWatchedObjectNodeLineItem::GetDescription() const
{
	if (UMDFastBindingObject* WatchedObject = WatchedObjectPtr.Get())
	{
		return WatchedObject->GetDisplayName();
	}
	
	return INVTEXT("[Invalid]");
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
					if (!CachedPropertyItems.Contains(ChildProp->GetFName()))
					{
						CachedPropertyItems.Add(ChildProp->GetFName(), MakeShared<FMDFastBindingPropertyDebugLineItem>(ChildProp, ChildValuePtr));
					}
					else
					{
						TSharedPtr<FMDFastBindingPropertyDebugLineItem> Item = StaticCastSharedPtr<FMDFastBindingPropertyDebugLineItem>(CachedPropertyItems[ChildProp->GetFName()]);
						if (Item->GetValuePtr() != ChildValuePtr)
						{
							Item->UpdateValuePtr(ChildValuePtr);
						}
					}
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
					const FName PropertyName = *FString::Printf(TEXT("%d"), i);
					if (!CachedPropertyItems.Contains(PropertyName))
					{
						CachedPropertyItems.Add(PropertyName, MakeShared<FMDFastBindingPropertyDebugLineItem>(ArrayProp->Inner, ChildValuePtr, ElementDisplayName));
					}
					else
					{
						TSharedPtr<FMDFastBindingPropertyDebugLineItem> Item = StaticCastSharedPtr<FMDFastBindingPropertyDebugLineItem>(CachedPropertyItems[PropertyName]);
						if (Item->GetValuePtr() != ChildValuePtr)
						{
							Item->UpdateValuePtr(ChildValuePtr);
						}
					}
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
					const FName PropertyName = *FString::Printf(TEXT("%d"), i);
					if (!CachedPropertyItems.Contains(PropertyName))
					{
						CachedPropertyItems.Add(PropertyName, MakeShared<FMDFastBindingPropertyDebugLineItem>(SetProp->ElementProp, ChildValuePtr, ElementDisplayName));
					}
					else
					{
						TSharedPtr<FMDFastBindingPropertyDebugLineItem> Item = StaticCastSharedPtr<FMDFastBindingPropertyDebugLineItem>(CachedPropertyItems[PropertyName]);
						if (Item->GetValuePtr() != ChildValuePtr)
						{
							Item->UpdateValuePtr(ChildValuePtr);
						}
					}
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
					const FName KeyPropertyName = *FString::Printf(TEXT("Key[%d]"), i);
					if (!CachedPropertyItems.Contains(KeyPropertyName))
					{
						CachedPropertyItems.Add(KeyPropertyName, MakeShared<FMDFastBindingPropertyDebugLineItem>(MapProp->KeyProp, ChildKeyPtr, KeyDisplayName));
					}
					else
					{
						TSharedPtr<FMDFastBindingPropertyDebugLineItem> Item = StaticCastSharedPtr<FMDFastBindingPropertyDebugLineItem>(CachedPropertyItems[KeyPropertyName]);
						if (Item->GetValuePtr() != ChildKeyPtr)
						{
							Item->UpdateValuePtr(ChildKeyPtr);
						}
					}

					// Value
					void* ChildValuePtr = Helper.GetValuePtr(i);
					const FText ValueDisplayName = FText::Format(INVTEXT("Value[{0}]"), FText::AsNumber(i));
					const FName ValuePropertyName = *FString::Printf(TEXT("Value[%d]"), i);
					if (!CachedPropertyItems.Contains(ValuePropertyName))
					{
						CachedPropertyItems.Add(ValuePropertyName, MakeShared<FMDFastBindingPropertyDebugLineItem>(MapProp->ValueProp, ChildValuePtr, ValueDisplayName));
					}
					else
					{
						TSharedPtr<FMDFastBindingPropertyDebugLineItem> Item = StaticCastSharedPtr<FMDFastBindingPropertyDebugLineItem>(CachedPropertyItems[ValuePropertyName]);
						if (Item->GetValuePtr() != ChildValuePtr)
						{
							Item->UpdateValuePtr(ChildValuePtr);
						}
					}
				}
			}
		}
	}
	
	CachedPropertyItems.GenerateValueArray(CachedChildren.GetValue());
}

void FMDFastBindingItemDebugLineItem::RefreshDebugObject(UMDFastBindingObject* DebugObject)
{
	DebugObjectPtr = DebugObject;
	CachedChildren.Reset();
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

UObject* FMDFastBindingItemDebugLineItem::GetParentObject()
{
	return DebugObjectPtr.Get();
}

void FMDFastBindingItemDebugLineItem::ExtendContextMenu(FMenuBuilder& MenuBuilder, bool bInDebuggerTab)
{
	FMDFastBindingDebugLineItem::ExtendContextMenu(MenuBuilder, bInDebuggerTab);

	if (const UMDFastBindingObject* DebugObject = DebugObjectPtr.Get())
	{
		if (UMDFastBindingEditorPersistantData::Get().IsPinBeingWatched(DebugObject->BindingObjectIdentifier, ItemName))
		{
			MenuBuilder.AddMenuEntry(
				INVTEXT("Remove Watch"),
				INVTEXT("Remove this pin from the Watch window"),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateWeakLambda(DebugObject, [Obj = DebugObjectPtr, PinName = ItemName]()
					{
						if (const UMDFastBindingObject* DebugObject = Obj.Get())
						{
							UMDFastBindingEditorPersistantData::Get().RemovePinFromWatchList(DebugObject->BindingObjectIdentifier, PinName);
						}
					}),
					FCanExecuteAction()
				)
			);
		}
		else
		{
			MenuBuilder.AddMenuEntry(
				INVTEXT("Add Watch"),
				INVTEXT("Add this pin to the Watch window"),
				FSlateIcon(),
				FUIAction(
					FExecuteAction::CreateWeakLambda(DebugObject, [Obj = DebugObjectPtr, PinName = ItemName]()
					{
						if (const UMDFastBindingObject* DebugObject = Obj.Get())
						{
							UMDFastBindingEditorPersistantData::Get().AddPinToWatchList(DebugObject->BindingObjectIdentifier, PinName);
						}
					}),
					FCanExecuteAction()
				)
			);
		}
	}
}


void FMDFastBindingPropertyDebugLineItem::UpdateValuePtr(void* InValuePtr)
{
	ValuePtr = InValuePtr;
	CachedChildren.Reset();
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

void SMDFastBindingWatchList::SetReferences(UMDFastBindingInstance* InCDOBinding, UMDFastBindingInstance* InDebugBinding)
{
	CDOBinding = InCDOBinding;
	DebugBinding = InDebugBinding;
}

void SMDFastBindingWatchList::RefreshList()
{
	// Hack to refresh the list since it's not exposed
	SetPinRef({});
}

void SMDFastBindingWatchList::PopulateTreeView()
{
	// Fallback to CDO binding so we can still see a list while not debugging
	bIsDebugging = DebugBinding.IsValid();
	if (const UMDFastBindingInstance* Binding = bIsDebugging ? DebugBinding.Get() : CDOBinding.Get())
	{
		const TArray<UMDFastBindingObject*> Objects = Binding->GatherAllBindingObjects();
		for (UMDFastBindingObject* Object : Objects)
		{
			if (UMDFastBindingEditorPersistantData::Get().IsNodeBeingWatched(Object->BindingObjectIdentifier))
			{
				TSharedPtr<FMDFastBindingWatchedObjectNodeLineItem>& Item = TreeItems.FindOrAdd(Object->BindingObjectIdentifier);
				if (!Item.IsValid())
				{
					Item = MakeShared<FMDFastBindingWatchedObjectNodeLineItem>(Object);
				}
				else
				{
					Item->RefreshWatchedObject(Object);
				}
				
				AddTreeItemUnique(Item);
			}
		}
	}
}

void SMDFastBindingWatchList::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SPinValueInspector::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	if (bIsDebugging && !DebugBinding.IsValid())
	{
		RefreshList();
	}
}
