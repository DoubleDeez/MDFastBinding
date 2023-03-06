#pragma once

#include "CoreMinimal.h"
#include "DragAndDrop/DecoratedDragDropOp.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STableRow.h"

class SInlineEditableTextBlock;
class SMDFastBindingEditorWidget;
class UMDFastBindingInstance;

class FMDFastBindingInstanceRowDragDropOp : public FDecoratedDragDropOp
{
public:
	DRAG_DROP_OPERATOR_TYPE(FBindingRowDragDropOp, FDecoratedDragDropOp)

	FMDFastBindingInstanceRowDragDropOp(TWeakObjectPtr<UMDFastBindingInstance> BindingPtr);

	void Init();

	void SetValidTarget(bool IsValidTarget);

	bool IsValidTarget(TWeakObjectPtr<UMDFastBindingInstance> TargetBindingPtr, const FVector2D& ScreenSpacePos, const FGeometry& Geometry) const;

	static EItemDropZone CalculateDropZoneRelativeToGeometry(const FVector2D& ScreenSpacePos, const FGeometry& Geometry);

	static int32 CalculateDropIndex(EItemDropZone DropZone, int32 TargetIndex);
	
	static int32 CalculateDropIndex(const FVector2D& ScreenSpacePos, const FGeometry& Geometry, int32 TargetIndex);
	
	static int32 CalculateDropResultIndex(EItemDropZone DropZone, int32 TargetIndex, int32 DraggedIndex);
	
	TWeakObjectPtr<UMDFastBindingInstance> CachedBindingPtr;
};


class SMDFastBindingInstanceRow : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SMDFastBindingInstanceRow)
		{
		}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, TSharedRef<SMDFastBindingEditorWidget> EditorWidget, TWeakObjectPtr<UMDFastBindingInstance> BindingPtr);

	void StartEditingTitle() const;

	virtual void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	virtual FReply OnMouseButtonDown(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

private:
	void SetBindingDisplayName(const FText& InName, ETextCommit::Type CommitType);
	
	FText GetBindingValidationTooltip() const;
	const FSlateBrush* GetBindingValidationBrush() const;

	FText GetBindingPerformanceTooltip() const;
	const FSlateBrush* GetBindingPerformanceBrush() const;
	
	const FSlateBrush* GetBorder() const;

	EVisibility GetHandleVisibility() const;
	
	TSharedPtr<SInlineEditableTextBlock> TitleText;
	TWeakObjectPtr<UMDFastBindingInstance> CachedBindingPtr;
	TWeakPtr<SMDFastBindingEditorWidget> CachedEditorWidget;
};
