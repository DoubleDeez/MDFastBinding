#pragma once

#include "UObject/NameTypes.h"
#include "Styling/SlateStyle.h"

/**
 *
 */
class MDFASTBINDINGEDITOR_API FMDFastBindingEditorStyle
{
public:
	static void Initialize();

	static void Shutdown();

	static const ISlateStyle& Get();

	static FName GetStyleSetName();

private:
	static TSharedRef<FSlateStyleSet> Create();

	static TSharedPtr<FSlateStyleSet> StyleInstance;
};
