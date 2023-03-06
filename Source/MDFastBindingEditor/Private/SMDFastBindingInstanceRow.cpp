#include "SMDFastBindingInstanceRow.h"

#include "MDFastBindingEditorStyle.h"
#include "MDFastBindingInstance.h"
#include "ScopedTransaction.h"
#include "SMDFastBindingEditorWidget.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/SInlineEditableTextBlock.h"

#define LOCTEXT_NAMESPACE "MDFastbindingInstanceRow"

class SMDFastBindingInstanceRowHandle : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMDFastBindingInstanceRowHandle)
	{
	}
	SLATE_ARGUMENT(TWeakObjectPtr<UMDFastBindingInstance>, BindingPtr)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs)
	{
		CachedBindingPtr = InArgs._BindingPtr;
		
		ChildSlot
		[
			SNew(SBox)
			.Padding(0.0f)
			.HAlign(HAlign_Center)
			.VAlign(VAlign_Center)
			.WidthOverride(16.0f)
			[
				SNew(SImage)
				.Image(FAppStyle::Get().GetBrush("VerticalBoxDragIndicatorShort"))
			]
		];
	}

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		return FReply::Handled().DetectDrag(SharedThis(this), EKeys::LeftMouseButton);
	};

	virtual FReply OnDragDetected(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override
	{
		if (MouseEvent.IsMouseButtonDown(EKeys::LeftMouseButton))
		{
			const TSharedPtr<FMDFastBindingInstanceRowDragDropOp> BindingDragOp = MakeShareable(new FMDFastBindingInstanceRowDragDropOp(CachedBindingPtr));
			BindingDragOp->Init();
			
			return FReply::Handled().BeginDragDrop(BindingDragOp.ToSharedRef());
		}

		return FReply::Unhandled();
	}

private:
	TWeakObjectPtr<UMDFastBindingInstance> CachedBindingPtr;
};

FMDFastBindingInstanceRowDragDropOp::FMDFastBindingInstanceRowDragDropOp(TWeakObjectPtr<UMDFastBindingInstance> BindingPtr)
	: CachedBindingPtr(BindingPtr)
{
	MouseCursor = EMouseCursor::GrabHandClosed;
}

void FMDFastBindingInstanceRowDragDropOp::Init()
{
	SetValidTarget(false);
	SetupDefaults();
	Construct();
}

void FMDFastBindingInstanceRowDragDropOp::SetValidTarget(bool IsValidTarget)
{
	if (IsValidTarget)
	{
		CurrentHoverText = LOCTEXT("MoveBindingHere", "Move Binding Here");
		CurrentIconBrush = FAppStyle::GetBrush("Graph.ConnectorFeedback.OK");
	}
	else
	{
		CurrentHoverText = LOCTEXT("CannotMoveBindingHere", "Cannot Move Binding Here");
		CurrentIconBrush = FAppStyle::GetBrush("Graph.ConnectorFeedback.Error");
	}
}

bool FMDFastBindingInstanceRowDragDropOp::IsValidTarget(TWeakObjectPtr<UMDFastBindingInstance> TargetBindingPtr, const FVector2D& ScreenSpacePos, const FGeometry& Geometry) const
{
	if (const UMDFastBindingInstance* TargetBinding = TargetBindingPtr.Get())
	{
		const int32 TargetBindingIndex = TargetBinding->GetBindingIndex();
		if (TargetBindingIndex != INDEX_NONE)
		{
			if (const UMDFastBindingInstance* DraggedBinding = CachedBindingPtr.Get())
			{
				const EItemDropZone OverrideDropZone = CalculateDropZoneRelativeToGeometry(ScreenSpacePos, Geometry);
				const int32 DropResultIndex = CalculateDropResultIndex(OverrideDropZone, TargetBindingIndex, DraggedBinding->GetBindingIndex());
				return DropResultIndex != INDEX_NONE && DropResultIndex != DraggedBinding->GetBindingIndex();
			}
		}
	}

	return false;
}

EItemDropZone FMDFastBindingInstanceRowDragDropOp::CalculateDropZoneRelativeToGeometry(const FVector2D& ScreenSpacePos, const FGeometry& Geometry)
{
	// Calculate if the dropzone is above or below the center of the geometry (directly onto the binding doesn't make sense)
	const float LocalPointerY = Geometry.AbsoluteToLocal(ScreenSpacePos).Y;
	return LocalPointerY < Geometry.GetLocalSize().Y * 0.5f ? EItemDropZone::AboveItem : EItemDropZone::BelowItem;
}

int32 FMDFastBindingInstanceRowDragDropOp::CalculateDropIndex(EItemDropZone DropZone, int32 TargetIndex)
{
	return DropZone == EItemDropZone::AboveItem ? TargetIndex : (TargetIndex + 1);
}

int32 FMDFastBindingInstanceRowDragDropOp::CalculateDropIndex(const FVector2D& ScreenSpacePos, const FGeometry& Geometry, int32 TargetIndex)
{
	return CalculateDropIndex(CalculateDropZoneRelativeToGeometry(ScreenSpacePos, Geometry), TargetIndex);
}

int32 FMDFastBindingInstanceRowDragDropOp::CalculateDropResultIndex(EItemDropZone DropZone, int32 TargetIndex, int32 DraggedIndex)
{
	const int32 DropIndex = CalculateDropIndex(DropZone, TargetIndex);
	int32 ResultIndex = INDEX_NONE;
		
	if (DraggedIndex > DropIndex)
	{
		ResultIndex = DropIndex;
	}
	else if (DraggedIndex < DropIndex)
	{
		ResultIndex = DropIndex - 1;
	}

	return ResultIndex;
}

void SMDFastBindingInstanceRow::Construct(const FArguments& InArgs, TSharedRef<SMDFastBindingEditorWidget> EditorWidget, TWeakObjectPtr<UMDFastBindingInstance> BindingPtr)
{
	CachedEditorWidget = EditorWidget;
	CachedBindingPtr = BindingPtr;
	
	if (UMDFastBindingInstance* Binding = BindingPtr.Get())
	{
		ChildSlot
		[
			SNew(SBorder)
			.Padding(3.f)
			.BorderImage(this, &SMDFastBindingInstanceRow::GetBorder)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Left)
				.AutoWidth()
				.Padding(0)
				[
					SNew(SMDFastBindingInstanceRowHandle)
					.BindingPtr(CachedBindingPtr)
					.Cursor(EMouseCursor::GrabHand)
					.Visibility(this, &SMDFastBindingInstanceRow::GetHandleVisibility)
				]
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Fill)
				.AutoWidth()
				.Padding(4.f, 0.f)
				[
					SNew(SImage)
					.ToolTipText(this, &SMDFastBindingInstanceRow::GetBindingValidationTooltip)
					.Image(this, &SMDFastBindingInstanceRow::GetBindingValidationBrush)
				]
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Fill)
				.AutoWidth()
				.Padding(4.f, 0.f)
				[
					SNew(SImage)
					.ToolTipText(this, &SMDFastBindingInstanceRow::GetBindingPerformanceTooltip)
					.Image(this, &SMDFastBindingInstanceRow::GetBindingPerformanceBrush)
				]
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Fill)
				.FillWidth(1.f)
				[
					SAssignNew(TitleText, SInlineEditableTextBlock)
					.Text_UObject(Binding, &UMDFastBindingInstance::GetBindingDisplayName)
					.OnTextCommitted(this, &SMDFastBindingInstanceRow::SetBindingDisplayName)
				]
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Fill)
				.AutoWidth()
				.Padding(2.f)
				[
					SNew(SButton)
					.ButtonStyle(FMDFastBindingEditorStyle::Get(), "BindingButton")
					.ContentPadding(2.f)
					.OnClicked(EditorWidget, &SMDFastBindingEditorWidget::OnDuplicateBinding, BindingPtr)
					.ToolTipText(LOCTEXT("DuplicateBindingTooltip", "Create a duplicate of this binding"))
					[
						SNew(SImage)
						.Image(FAppStyle::Get().GetBrush(TEXT("GenericCommands.Duplicate")))
					]
				]
				+SHorizontalBox::Slot()
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Fill)
				.AutoWidth()
				.Padding(2.f)
				[
					SNew(SButton)
					.ButtonStyle(FMDFastBindingEditorStyle::Get(), "BindingButton")
					.ContentPadding(2.f)
					.OnClicked(EditorWidget, &SMDFastBindingEditorWidget::OnDeleteBinding, BindingPtr)
					.ToolTipText(LOCTEXT("DeleteBindingTooltip", "Delete this binding"))
					[
						SNew(SImage)
						.Image(FAppStyle::Get().GetBrush(TEXT("GenericCommands.Delete")))
					]
				]
			]
		];
	}
}

void SMDFastBindingInstanceRow::StartEditingTitle() const
{
	if (TitleText.IsValid())
	{
		TitleText->EnterEditingMode();
	}
}

void SMDFastBindingInstanceRow::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	// Focus the newly created binding's name field
	if (const TSharedPtr<SMDFastBindingEditorWidget> EditorWidget = CachedEditorWidget.Pin())
	{
		if (EditorWidget->GetNewBinding() == CachedBindingPtr)
		{
			StartEditingTitle();
			EditorWidget->ResetNewBinding();
		}
	}
}

FReply SMDFastBindingInstanceRow::OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	if (MouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (CachedEditorWidget.Pin().IsValid())
		{
			CachedEditorWidget.Pin()->SelectBinding(CachedBindingPtr.Get());
		}
	}

	return SCompoundWidget::OnMouseButtonDown(MyGeometry, MouseEvent);
}

void SMDFastBindingInstanceRow::SetBindingDisplayName(const FText& InName, ETextCommit::Type CommitType)
{
	if (UMDFastBindingInstance* BindingPtr = CachedBindingPtr.Get())
	{
		FScopedTransaction Transaction = FScopedTransaction(LOCTEXT("RenameBindingTransaction", "Renamed Binding"));
		BindingPtr->SetBindingDisplayName(InName);
	}
}

FText SMDFastBindingInstanceRow::GetBindingValidationTooltip() const
{
	if (UMDFastBindingInstance* BindingPtr = CachedBindingPtr.Get())
	{
		TArray<FText> Errors;
		BindingPtr->IsDataValid(Errors);
		return FText::Join(FText::FromString(TEXT("\n")), Errors);
	}

	return FText::GetEmpty();
}

const FSlateBrush* SMDFastBindingInstanceRow::GetBindingValidationBrush() const
{
	if (UMDFastBindingInstance* BindingPtr = CachedBindingPtr.Get())
	{
		TArray<FText> Errors;
		const EDataValidationResult Result = BindingPtr->IsDataValid(Errors);
		if (Result == EDataValidationResult::Valid)
		{
			return FAppStyle::Get().GetBrush("Icons.SuccessWithColor");
		}
		else if (Result == EDataValidationResult::Invalid)
		{
			return FAppStyle::Get().GetBrush("Icons.ErrorWithColor");
		}
	}

	return nullptr;
}

FText SMDFastBindingInstanceRow::GetBindingPerformanceTooltip() const
{
	if (const UMDFastBindingInstance* BindingPtr = CachedBindingPtr.Get())
	{
		if (BindingPtr->IsBindingPerformant())
		{
			return LOCTEXT("PerformantBindingTooltip", "This binding does not run every tick");
		}
		else
		{
			return LOCTEXT("PerformantBindingTooltip", "This binding is evaluated every tick");
		}
	}
	
	return FText::GetEmpty();
}

const FSlateBrush* SMDFastBindingInstanceRow::GetBindingPerformanceBrush() const
{
	if (const UMDFastBindingInstance* BindingPtr = CachedBindingPtr.Get())
	{
		if (BindingPtr->IsBindingPerformant())
		{
			return FMDFastBindingEditorStyle::Get().GetBrush("Icon.Flame");
		}
		else
		{
			return FMDFastBindingEditorStyle::Get().GetBrush("Icon.Clock");
		}
	}
	
	return nullptr;
}

const FSlateBrush* SMDFastBindingInstanceRow::GetBorder() const
{
	if (CachedEditorWidget.Pin().IsValid() && CachedEditorWidget.Pin()->GetSelectedBinding() == CachedBindingPtr)
	{
		return FMDFastBindingEditorStyle::Get().GetBrush(TEXT("Background.Selector"));
	}

	return FMDFastBindingEditorStyle::Get().GetBrush(TEXT("Background.SelectorInactive"));
}

EVisibility SMDFastBindingInstanceRow::GetHandleVisibility() const
{
	return IsHovered() ? EVisibility::Visible : EVisibility::Hidden;
}

#undef LOCTEXT_NAMESPACE
