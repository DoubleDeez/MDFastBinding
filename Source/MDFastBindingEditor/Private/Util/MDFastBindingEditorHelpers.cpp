#include "Util/MDFastBindingEditorHelpers.h"

#include "MDFastBindingContainer.h"
#include "WidgetBlueprint.h"
#include "WidgetBlueprintExtension.h"
#include "WidgetExtension/MDFastBindingWidgetBlueprintExtension.h"
#include "WidgetExtension/MDFastBindingWidgetExtension.h"


bool MDFastBindingEditorHelpers::DoesObjectSupportFastBindings(const UObject& Object)
{
	if (const UBlueprint* BP = Cast<UBlueprint>(&Object))
	{
		return DoesClassSupportFastBindings(BP->GeneratedClass);
	}

	return DoesClassSupportFastBindings(Object.GetClass());
}

bool MDFastBindingEditorHelpers::DoesClassSupportFastBindings(const UStruct* Class)
{
	if (Class != nullptr && Class->IsA<UWidgetBlueprintGeneratedClass>())
	{
		// All UserWidgets support fast bindings
		return true;
	}
	
	TSet<const UStruct*> VisitedClasses;
	TArray<const UStruct*> ClassesToVisit = { Class };

	while (ClassesToVisit.Num() > 0)
	{
		const UStruct* ClassToVisit = ClassesToVisit.Pop();
		VisitedClasses.Add(ClassToVisit);
		
		for (TFieldIterator<FProperty> It(ClassToVisit); It; ++It)
		{
			if (const FObjectPropertyBase* ObjectProp = CastField<const FObjectPropertyBase>(*It))
			{
				if (ObjectProp->PropertyClass->IsChildOf(UMDFastBindingContainer::StaticClass()))
				{
					return true;
				}
				
				if (!VisitedClasses.Contains(ObjectProp->PropertyClass))
				{
					ClassesToVisit.Push(ObjectProp->PropertyClass);
				}
			}
			else if (const FStructProperty* StructProp = CastField<const FStructProperty>(*It))
			{
				if (!VisitedClasses.Contains(StructProp->Struct))
				{
					ClassesToVisit.Push(StructProp->Struct);
				}
			}
		}
	}
	
	return false;
}

UMDFastBindingContainer* MDFastBindingEditorHelpers::FindBindingContainerInObject(UObject* Object)
{
	if (const UUserWidget* Widget = Cast<UUserWidget>(Object))
	{
		if (const UMDFastBindingWidgetExtension* Extension = Widget->GetExtension<UMDFastBindingWidgetExtension>())
		{
			return Extension->GetBindingContainer();
		}
	}

	if (Object != nullptr)
	{
		for (TFieldIterator<FObjectPropertyBase> It(Object->GetClass()); It; ++It)
		{
			if (It->PropertyClass->IsChildOf(UMDFastBindingContainer::StaticClass()))
			{
				if (UMDFastBindingContainer* Container = Cast<UMDFastBindingContainer>(It->GetObjectPropertyValue_InContainer(Object)))
				{
					return Container;
				}
			}
		}	
	}

	return nullptr;
}

UMDFastBindingContainer* MDFastBindingEditorHelpers::FindBindingContainerCDOInClass(UClass* Class)
{
	if (Class != nullptr)
	{		
		for (TFieldIterator<FObjectPropertyBase> It(Class); It; ++It)
		{
			if (It->PropertyClass->IsChildOf(UMDFastBindingContainer::StaticClass()))
			{
				if (const UObject* BindingOwnerCDO = Class->GetDefaultObject())
				{
					if (UMDFastBindingContainer* Container = Cast<UMDFastBindingContainer>(It->GetObjectPropertyValue_InContainer(BindingOwnerCDO)))
					{
						return Container;
					}
				}
			}
		}
	}

	return nullptr;
}

UMDFastBindingContainer* MDFastBindingEditorHelpers::FindBindingContainerCDOInBlueprint(UBlueprint* Blueprint)
{
	if (Blueprint != nullptr)
	{
		if (const UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(Blueprint))
		{
			if (const UMDFastBindingWidgetBlueprintExtension* FastBindingExtension = UMDFastBindingWidgetBlueprintExtension::GetExtension<UMDFastBindingWidgetBlueprintExtension>(WidgetBP))
			{
				return FastBindingExtension->GetBindingContainer();
			}
		}

		return FindBindingContainerCDOInClass(Blueprint->GeneratedClass);
	}
	
	return nullptr;
}

UMDFastBindingContainer* MDFastBindingEditorHelpers::InitBindingContainerInBlueprint(UBlueprint* Blueprint, UMDFastBindingContainer* InitialContainer)
{
	if (Blueprint != nullptr)
	{
		if (UWidgetBlueprint* WidgetBP = Cast<UWidgetBlueprint>(Blueprint))
		{
			UMDFastBindingWidgetBlueprintExtension* FastBindingExtension = UMDFastBindingWidgetBlueprintExtension::RequestExtension<UMDFastBindingWidgetBlueprintExtension>(WidgetBP);
			UMDFastBindingContainer* Container = InitialContainer == nullptr
				? NewObject<UMDFastBindingContainer>(FastBindingExtension, NAME_None, RF_Transactional | RF_Public)
				: DuplicateObject(InitialContainer, FastBindingExtension);
			FastBindingExtension->SetBindingContainer(Container);
			
			return Container;
		}

		return InitBindingContainerInClass(Blueprint->GeneratedClass);
	}
	
	return nullptr;
}

UMDFastBindingContainer* MDFastBindingEditorHelpers::InitBindingContainerInClass(UClass* Class)
{
	if (Class != nullptr)
	{		
		for (TFieldIterator<FObjectPropertyBase> It(Class); It; ++It)
		{
			if (It->PropertyClass->IsChildOf(UMDFastBindingContainer::StaticClass()))
			{
				if (UObject* BindingOwnerCDO = Class->GetDefaultObject())
				{
					UMDFastBindingContainer* NewContainer = NewObject<UMDFastBindingContainer>(BindingOwnerCDO, NAME_None, RF_Transactional | RF_Public);
					It->SetObjectPropertyValue_InContainer(BindingOwnerCDO, NewContainer);
					return NewContainer;
				}
			}
		}
	}

	return nullptr;
}

void MDFastBindingEditorHelpers::ClearBindingContainerCDOInClass(UClass* Class)
{
	if (Class != nullptr)
	{		
		for (TFieldIterator<FObjectPropertyBase> It(Class); It; ++It)
		{
			if (It->PropertyClass->IsChildOf(UMDFastBindingContainer::StaticClass()))
			{
				if (UObject* BindingOwnerCDO = Class->GetDefaultObject())
				{
					It->SetObjectPropertyValue_InContainer(BindingOwnerCDO, nullptr);
				}
			}
		}
	}
}
