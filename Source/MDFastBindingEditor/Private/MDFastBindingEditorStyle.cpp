#include "MDFastBindingEditorStyle.h"

#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/StyleColors.h"

TSharedPtr<FSlateStyleSet> FMDFastBindingEditorStyle::StyleInstance = nullptr;

void FMDFastBindingEditorStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FMDFastBindingEditorStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

const ISlateStyle& FMDFastBindingEditorStyle::Get()
{
	return *StyleInstance;
}

FName FMDFastBindingEditorStyle::GetStyleSetName()
{
	return TEXT("MDFastBindingEditorStyle");
}

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( Style->RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( Style->RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( Style->RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__ )
#define DEFAULT_FONT(...) FCoreStyle::GetDefaultFontStyle(__VA_ARGS__)

const FVector2D Icon16x16(16.0f, 16.0f);
const FVector2D Icon24x24(24.0f, 24.0f);

TSharedRef<FSlateStyleSet> FMDFastBindingEditorStyle::Create()
{
	TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet(GetStyleSetName()));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin(TEXT("MDFastBinding"))->GetBaseDir() / TEXT("Resources"));
#if ENGINE_MAJOR_VERSION <= 4
	Style->Set(TEXT("Icon.Check"), const_cast<FSlateBrush*>(&FCoreStyle::Get().GetWidgetStyle<FCheckBoxStyle>(TEXT("Menu.Check")).CheckedImage));
#else
	Style->Set(TEXT("Icon.Check"), const_cast<FSlateBrush*>(&FAppStyle::Get().GetWidgetStyle<FCheckBoxStyle>(TEXT("Menu.Check")).CheckedImage));
#endif
	Style->Set(TEXT("Icon.FastBinding_16x"), new IMAGE_BRUSH(TEXT("FastBindingIcon_16x"), Icon16x16));
	Style->Set(TEXT("Icon.FastBinding_24x"), new IMAGE_BRUSH(TEXT("FastBindingIcon_24x"), Icon24x24));

	Style->Set(TEXT("Background.Selector"), new FSlateColorBrush(FStyleColors::Select.GetSpecifiedColor() * 0.5f));
	Style->Set(TEXT("Background.SelectorInactive"), new FSlateColorBrush(FLinearColor::Transparent));
	
#if ENGINE_MAJOR_VERSION <= 4
	FButtonStyle ButtonStyle = FCoreStyle::Get().GetWidgetStyle< FButtonStyle >("FlatButton");
#else
	FButtonStyle ButtonStyle = FAppStyle::Get().GetWidgetStyle< FButtonStyle >("FlatButton");
#endif
	ButtonStyle.SetNormalPadding(FMargin(2.f));
	ButtonStyle.SetPressedPadding(FMargin(2.f));
	Style->Set(TEXT("BindingButton"), ButtonStyle);

	Style->Set(TEXT("NodeTitleColor"), FLinearColor(FColorList::Cyan));
	Style->Set(TEXT("DestinationNodeTitleColor"), FLinearColor::Green);
	Style->Set(TEXT("InactiveDestinationNodeTitleColor"), FLinearColor::Green * 0.125f);
	Style->Set(TEXT("InvalidPinColor"), FLinearColor(FColorList::OrangeRed));

	return Style;
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef DEFAULT_FONT
