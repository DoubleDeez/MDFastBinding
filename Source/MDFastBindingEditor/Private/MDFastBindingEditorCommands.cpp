#include "MDFastBindingEditorCommands.h"


#define LOCTEXT_NAMESPACE "MDFastBindingEditorCommands"

void FMDFastBindingEditorCommands::RegisterCommands()
{
	UI_COMMAND(SetDestinationActive, "Set Active", "Set Destination Active", EUserInterfaceActionType::Button, FInputChord() )
}

#undef LOCTEXT_NAMESPACE
