#include "BindingValues/MDFastBindingValue_FormatText.h"

#include "MDFastBinding.h"

TTuple<const FProperty*, void*> UMDFastBindingValue_FormatText::GetValue(UObject* SourceObject)
{
#if !WITH_EDITOR
	if (!TextFormat.IsValid())
#endif
	{
		TextFormat = FormatText;
	}
	
	Args.Empty(Arguments.Num());

	for (const FName& Arg : Arguments)
	{
		const TTuple<const FProperty*, void*> ArgValue = GetBindingItemValue(SourceObject, Arg);
		if (ArgValue.Value != nullptr)
		{
			FText ArgText;
			FMDFastBindingModule::SetProperty(GetOutputProperty(), &ArgText, ArgValue.Key, ArgValue.Value);

			Args.Add(Arg.ToString(), ArgText);
		}
		else
		{
			Args.Add(Arg.ToString(), FText::GetEmpty());
		}
	}

	OutputValue = FText::Format(TextFormat, Args);
	
	return TTuple<const FProperty*, void*>{ GetOutputProperty(), &OutputValue };
}

const FProperty* UMDFastBindingValue_FormatText::GetOutputProperty()
{
	if (TextProp == nullptr)
	{
		TextProp = GetClass()->FindPropertyByName(GET_MEMBER_NAME_CHECKED(UMDFastBindingValue_FormatText, OutputValue));	
	}
	
	return TextProp;
}

void UMDFastBindingValue_FormatText::SetupBindingItems()
{
	Arguments.Empty();
	
	FString Argument;
	Argument.Reserve(FormatText.ToString().Len());
	
	bool bIsParsingArgument = false;
	bool bIsEscaping = false;
	for (const TCHAR& Char : FormatText.ToString())
	{
		if (Char == '`' && !bIsEscaping)
		{
			bIsEscaping = true;
			continue;
		}

		if (Char == '{' && !bIsEscaping)
		{
			bIsParsingArgument = true;
		}
		else if (bIsParsingArgument && Char == '}' && !bIsEscaping)
		{
			bIsParsingArgument = false;
			Arguments.Emplace(MoveTemp(Argument));
			Argument.Empty(FormatText.ToString().Len());
		}
		else if (bIsParsingArgument)
		{
			Argument.AppendChar(Char);
		}

		bIsEscaping = false;
	}

	for (int32 i = BindingItems.Num() - 1; i >= 0; --i)
	{
		if (!Arguments.Contains(BindingItems[i].ItemName))
		{
#if WITH_EDITORONLY_DATA
			if (BindingItems[i].Value != nullptr)
			{
				OrphanBindingItem(BindingItems[i].Value);
			}
#endif
			BindingItems.RemoveAt(i);
		}
	}

	for (const FName& Arg : Arguments)
	{
		EnsureBindingItemExists(Arg, GetOutputProperty(), FText::GetEmpty());
	}
}
