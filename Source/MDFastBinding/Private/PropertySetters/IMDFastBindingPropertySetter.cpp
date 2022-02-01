#include "PropertySetters/IMDFastBindingPropertySetter.h"

bool IMDFastBindingPropertySetter::CanSetProperty(const FProperty& DestinationProp, const FProperty& SourceProp) const
{
	return GetSupportedFieldTypes().Contains(DestinationProp.GetClass())
		&& GetSupportedFieldTypes().Contains(SourceProp.GetClass());
}
