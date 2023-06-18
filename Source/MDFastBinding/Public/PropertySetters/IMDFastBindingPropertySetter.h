#pragma once

#include "Templates/SharedPointer.h"

class FFieldClass;
class FProperty;

/**
 *
 */
class MDFASTBINDING_API IMDFastBindingPropertySetter : public TSharedFromThis<IMDFastBindingPropertySetter>
{
public:
	virtual ~IMDFastBindingPropertySetter() = default;

	// Get all of supported field types this setter can handle
	virtual const TSet<FFieldClass*>& GetSupportedFieldTypes() const;

	// This function does the actual setting by taking the value held in SourceValuePtr, optionally doing any conversion, then setting the value in DestinationContainerPtr
	// Implementation can assume the CanSetProperty check has passed
	// It's expected for this to respect DestinationProp's Setter
	virtual void SetPropertyInContainer(const FProperty& DestinationProp, void* DestinationContainerPtr, const FProperty& SourceProp, const void* SourceValuePtr) const = 0;

	// This function does the actual setting by taking the value held in SourceValuePtr, optionally doing any conversion, then setting the value in DestinationValuePtr
	// Implementation can assume the CanSetProperty check has passed
	// It's not expected for this to respect DestinationProp's Setter
	virtual void SetPropertyDirectly(const FProperty& DestinationProp, void* DestinationValuePtr, const FProperty& SourceProp, const void* SourceValuePtr) const = 0;

	// Similar to GetSupportedFieldTypes, but specifically about 2 properties, default implementation checks GetSupportedFieldTypes for the 2 property types passed in
	virtual bool CanSetProperty(const FProperty& DestinationProp, const FProperty& SourceProp) const;
};
