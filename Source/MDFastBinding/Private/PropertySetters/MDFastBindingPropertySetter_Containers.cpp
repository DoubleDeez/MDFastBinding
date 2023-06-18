#include "PropertySetters/MDFastBindingPropertySetter_Containers.h"

#include "MDFastBinding.h"
#include "UObject/UnrealType.h"

namespace MDFastBindingPropertySetter_Containers_Private
{
	template<bool bIsDestinationAContainer>
	void SetPropertyImpl(const FProperty& DestinationProp, void* DestinationPtr, const FProperty& SourceProp, const void* SourceValuePtr)
	{
		void* DestinationContainerPtr = DestinationPtr;

		if constexpr (bIsDestinationAContainer)
		{
			// To respect the destination's setter, we have to operate on an intermediate value, so we allocate one here
			DestinationPtr = FMemory::Malloc(DestinationProp.GetSize(), DestinationProp.GetMinAlignment());
			DestinationProp.InitializeValue(DestinationPtr);
		}

		const FArrayProperty* DestArrayProp = CastField<const FArrayProperty>(&DestinationProp);
		const FArrayProperty* SrcArrayProp = CastField<const FArrayProperty>(&SourceProp);
		if (DestArrayProp != nullptr && SrcArrayProp != nullptr)
		{
			FScriptArrayHelper DestHelper = FScriptArrayHelper(DestArrayProp, DestinationPtr);
			FScriptArrayHelper SrcHelper = FScriptArrayHelper(SrcArrayProp, SourceValuePtr);

			const int32 NumSrcElements = SrcHelper.Num();
			const int32 NumDestElements = DestHelper.Num();
			if (NumSrcElements > NumDestElements)
			{
				DestHelper.AddValues(NumSrcElements - NumDestElements);
			}
			else if (NumSrcElements < NumDestElements)
			{
				DestHelper.RemoveValues(0, NumDestElements - NumSrcElements);
			}

			for (int32 i = 0; i < NumSrcElements; ++i)
			{
				const void* SrcValuePtr = SrcHelper.GetRawPtr(i);
				void* DestValuePtr = DestHelper.GetRawPtr(i);
				FMDFastBindingModule::SetPropertyDirectly(DestArrayProp->Inner, DestValuePtr, SrcArrayProp->Inner, SrcValuePtr);
			}

			return;
		}

		const FSetProperty* DestSetProp = CastField<const FSetProperty>(&DestinationProp);
		const FSetProperty* SrcSetProp = CastField<const FSetProperty>(&SourceProp);
		if (DestSetProp != nullptr && SrcSetProp != nullptr)
		{
			FScriptSetHelper DestHelper = FScriptSetHelper(DestSetProp, DestinationPtr);
			FScriptSetHelper SrcHelper = FScriptSetHelper(SrcSetProp, SourceValuePtr);

			const int32 NumSrcElements = SrcHelper.Num();
			DestHelper.EmptyElements(NumSrcElements);

			for (int32 i = 0; i < NumSrcElements; ++i)
			{
				DestHelper.AddDefaultValue_Invalid_NeedsRehash();
				const void* SrcValuePtr = SrcHelper.GetElementPtr(i);
				void* DestValuePtr = DestHelper.GetElementPtr(i);
				FMDFastBindingModule::SetPropertyDirectly(DestSetProp->ElementProp, DestValuePtr, SrcSetProp->ElementProp, SrcValuePtr);
			}

			DestHelper.Rehash();
			return;
		}

		const FMapProperty* DestMapProp = CastField<const FMapProperty>(&DestinationProp);
		const FMapProperty* SrcMapProp = CastField<const FMapProperty>(&SourceProp);
		if (DestMapProp != nullptr && SrcMapProp != nullptr)
		{
			FScriptMapHelper DestHelper = FScriptMapHelper(DestMapProp, DestinationPtr);
			FScriptMapHelper SrcHelper = FScriptMapHelper(SrcMapProp, SourceValuePtr);

			const int32 NumSrcElements = SrcHelper.Num();
			DestHelper.EmptyValues(NumSrcElements);

			for (int32 i = 0; i < NumSrcElements; ++i)
			{
				DestHelper.AddDefaultValue_Invalid_NeedsRehash();

				const void* SrcKeyPtr = SrcHelper.GetKeyPtr(i);
				void* DestKeyPtr = DestHelper.GetKeyPtr(i);
				FMDFastBindingModule::SetPropertyDirectly(DestSetProp->ElementProp, DestKeyPtr, SrcSetProp->ElementProp, SrcKeyPtr);

				const void* SrcValuePtr = SrcHelper.GetValuePtr(i);
				void* DestValuePtr = DestHelper.GetValuePtr(i);
				FMDFastBindingModule::SetPropertyDirectly(DestSetProp->ElementProp, DestValuePtr, SrcSetProp->ElementProp, SrcValuePtr);
			}

			DestHelper.Rehash();
		}

		if constexpr (bIsDestinationAContainer)
		{
			DestinationProp.SetValue_InContainer(DestinationContainerPtr, DestinationPtr);

			FMemory::Free(DestinationPtr);
			DestinationPtr = nullptr;
		}
	}
}

FMDFastBindingPropertySetter_Containers::FMDFastBindingPropertySetter_Containers()
	: SupportedFieldTypes({ FArrayProperty::StaticClass(), FSetProperty::StaticClass(), FMapProperty::StaticClass() })
{
}

void FMDFastBindingPropertySetter_Containers::SetPropertyInContainer(const FProperty& DestinationProp, void* DestinationContainerPtr, const FProperty& SourceProp, const void* SourceValuePtr) const
{
	MDFastBindingPropertySetter_Containers_Private::SetPropertyImpl<true>(DestinationProp, DestinationContainerPtr, SourceProp, SourceValuePtr);
}

void FMDFastBindingPropertySetter_Containers::SetPropertyDirectly(const FProperty& DestinationProp, void* DestinationValuePtr, const FProperty& SourceProp, const void* SourceValuePtr) const
{
	MDFastBindingPropertySetter_Containers_Private::SetPropertyImpl<false>(DestinationProp, DestinationValuePtr, SourceProp, SourceValuePtr);
}

bool FMDFastBindingPropertySetter_Containers::CanSetProperty(const FProperty& DestinationProp, const FProperty& SourceProp) const
{
	if (!IMDFastBindingPropertySetter::CanSetProperty(DestinationProp, SourceProp))
	{
		return false;
	}

	// Only match up same types of containers
	if (DestinationProp.GetClass() != SourceProp.GetClass())
	{
		return false;
	}

	// Identical container types should use the default implementation
	if (DestinationProp.SameType(&SourceProp))
	{
		return false;
	}

	const FArrayProperty* DestArrayProp = CastField<const FArrayProperty>(&DestinationProp);
	const FArrayProperty* SrcArrayProp = CastField<const FArrayProperty>(&SourceProp);
	if (DestArrayProp != nullptr && SrcArrayProp != nullptr)
	{
		return FMDFastBindingModule::CanSetProperty(DestArrayProp->Inner, SrcArrayProp->Inner);
	}

	const FSetProperty* DestSetProp = CastField<const FSetProperty>(&DestinationProp);
	const FSetProperty* SrcSetProp = CastField<const FSetProperty>(&SourceProp);
	if (DestSetProp != nullptr && SrcSetProp != nullptr)
	{
		return FMDFastBindingModule::CanSetProperty(DestSetProp->ElementProp, SrcSetProp->ElementProp);
	}

	const FMapProperty* DestMapProp = CastField<const FMapProperty>(&DestinationProp);
	const FMapProperty* SrcMapProp = CastField<const FMapProperty>(&SourceProp);
	if (DestMapProp != nullptr && SrcMapProp != nullptr)
	{
		return FMDFastBindingModule::CanSetProperty(DestMapProp->KeyProp, SrcMapProp->KeyProp)
			&& FMDFastBindingModule::CanSetProperty(DestMapProp->ValueProp, SrcMapProp->ValueProp);
	}

	return false;
}
