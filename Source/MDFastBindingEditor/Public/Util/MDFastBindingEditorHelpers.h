#pragma once

#include "MDFastBindingContainer.h"

class UMDFastBindingContainer;

namespace MDFastBindingEditorHelpers
{
	bool DoesObjectSupportFastBindings(const UObject& Object);
	bool DoesClassSupportFastBindings(const UStruct* Class);

	UMDFastBindingContainer* FindBindingContainerInObject(UObject* Object);

	UMDFastBindingContainer* FindBindingContainerCDOInClass(UClass* Class);
	UMDFastBindingContainer* FindBindingContainerCDOInBlueprint(UBlueprint* Blueprint);

	UMDFastBindingContainer* InitBindingContainerInBlueprint(UBlueprint* Blueprint, UMDFastBindingContainer* InitialContainer = nullptr);
	UMDFastBindingContainer* InitBindingContainerInClass(UClass* Class);

	void ClearBindingContainerCDOInClass(UClass* Class);
};
