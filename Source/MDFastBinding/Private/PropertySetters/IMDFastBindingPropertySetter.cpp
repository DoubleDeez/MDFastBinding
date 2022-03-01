#include "PropertySetters/IMDFastBindingPropertySetter.h"

#include "UObject/UnrealType.h"

const TSet<FFieldClass*>& IMDFastBindingPropertySetter::GetSupportedFieldTypes() const
{
	static const TSet<FFieldClass*> EmptySet;
	return EmptySet;
}

bool IMDFastBindingPropertySetter::CanSetProperty(const FProperty& DestinationProp, const FProperty& SourceProp) const
{
	return GetSupportedFieldTypes().Contains(DestinationProp.GetClass())
		&& GetSupportedFieldTypes().Contains(SourceProp.GetClass());
}
