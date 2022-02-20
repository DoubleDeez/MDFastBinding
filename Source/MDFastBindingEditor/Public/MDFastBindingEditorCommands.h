#pragma once

#include "CoreMinimal.h"
#include "Styling/CoreStyle.h"
#include "Framework/Commands/Commands.h"


class MDFASTBINDINGEDITOR_API FMDFastBindingEditorCommands : public TCommands<FMDFastBindingEditorCommands>
{
public:
	
	FMDFastBindingEditorCommands()
		: TCommands<FMDFastBindingEditorCommands>( TEXT("MDFastBindingEditorCommands"), NSLOCTEXT("MDFastBindingEditorCommands", "FastBindingCommands", "Fast Binding Commands"), NAME_None, FCoreStyle::Get().GetStyleSetName() )
	{
	}

	virtual ~FMDFastBindingEditorCommands()
	{
	}

	virtual void RegisterCommands() override;	

	TSharedPtr<FUICommandInfo> SetDestinationActive;	
};