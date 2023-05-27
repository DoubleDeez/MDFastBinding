#include "Graph/SMDFastBindingGraphNodeWidget.h"

#include "BlueprintEditor.h"
#include "Debug/MDFastBindingEditorDebug.h"
#include "GraphEditorSettings.h"
#include "Graph/MDFastBindingGraphNode.h"
#include "MDFastBindingObject.h"
#include "NodeFactory.h"
#include "SGraphPanel.h"
#include "Styling/SlateStyleRegistry.h"
#include "Widgets/SBoxPanel.h"

#define LOCTEXT_NAMESPACE "MDFastBindingGraphNodeWidget"

void SMDFastBindingGraphNodeWidget::Construct(const FArguments& InArgs, UMDFastBindingGraphNode* InNode)
{
	GraphNode = InNode;
	UpdateGraphNode();
}

void SMDFastBindingGraphNodeWidget::MoveTo(const FVector2D& NewPosition, FNodeSet& NodeFilter, bool bMarkDirty)
{
	SGraphNode::MoveTo(NewPosition, NodeFilter, bMarkDirty);

	if (UMDFastBindingGraphNode* Node = GetGraphNode())
	{
		Node->OnMoved();
	}
}

UMDFastBindingGraphNode* SMDFastBindingGraphNodeWidget::GetGraphNode() const
{
	return Cast<UMDFastBindingGraphNode>(GraphNode);
}

UMDFastBindingObject* SMDFastBindingGraphNodeWidget::GetBindingObject() const
{
	if (const UMDFastBindingGraphNode* Node = GetGraphNode())
	{
		return Node->GetBindingObject();
	}

	return nullptr;
}

UMDFastBindingObject* SMDFastBindingGraphNodeWidget::GetBindingObjectBeingDebugged() const
{
	if (const UMDFastBindingGraphNode* Node = GetGraphNode())
	{
		return Node->GetBindingObjectBeingDebugged();
	}

	return nullptr;
}

void SMDFastBindingGraphNodeWidget::UpdateErrorInfo()
{
	ErrorColor = FLinearColor(0,0,0);
	ErrorMsg.Empty();

	if (UMDFastBindingObject* BindingObject = GetBindingObject())
	{
		TArray<FText> Errors;
		if (BindingObject->IsDataValid(Errors) == EDataValidationResult::Invalid)
		{
			ErrorColor = FAppStyle::GetColor("ErrorReporting.BackgroundColor");

			if (Errors.Num() == 0)
			{
				ErrorMsg = LOCTEXT("InvalidWithoutErrors", "Unknown Error").ToString();
			}
			else
			{
				ErrorMsg = Errors.Last().ToString();
			}
		}
	}
}

TSharedPtr<SGraphPin> SMDFastBindingGraphNodeWidget::CreatePinWidget(UEdGraphPin* Pin) const
{
	if (IsSelfPin(*Pin))
	{
		return SNew(SMDFastBindingSelfGraphPinWidget, Pin);
	}
	else if (TSharedPtr<SGraphPin> K2Pin = FNodeFactory::CreateK2PinWidget(Pin))
	{
		return K2Pin;
	}

	return SGraphNode::CreatePinWidget(Pin);
}

void SMDFastBindingGraphNodeWidget::CreateBelowPinControls(TSharedPtr<SVerticalBox> MainBox)
{
	if (UMDFastBindingObject* BindingObject = GetBindingObject())
	{
		// HACK - this function implies we should add below the pins, but inserting at Slot 1 puts this widget right below the title bar
		MainBox->InsertSlot(1)
		.AutoHeight()
		.HAlign(HAlign_Fill)
		.VAlign(VAlign_Center)
		.Padding(10.f, 2.f)
		[
			BindingObject->CreateNodeHeaderWidget()
		];
	}
}

void SMDFastBindingGraphNodeWidget::CreateInputSideAddButton(TSharedPtr<SVerticalBox> InputBox)
{
	if (const UMDFastBindingObject* BindingObject = GetBindingObject())
	{
		if (BindingObject->HasUserExtendablePinList())
		{
			const TSharedRef<SWidget> AddPinButton = AddPinButtonContent(LOCTEXT("AddPinLabel", "Add Pin"), LOCTEXT("AddPinTooltip", "Add another input to this node"));

			FMargin AddPinPadding = Settings->GetOutputPinPadding();
			AddPinPadding.Top += 6.0f;

			InputBox->AddSlot()
				.AutoHeight()
				.VAlign(VAlign_Center)
				.Padding(AddPinPadding)
				[
					AddPinButton
				];
		}
	}
}

FReply SMDFastBindingGraphNodeWidget::OnAddPin()
{
	if (UMDFastBindingObject* BindingObject = GetBindingObject())
	{
		BindingObject->IncrementExtendablePinCount();
		if (UMDFastBindingGraphNode* Node = GetGraphNode())
		{
			Node->RefreshGraph();
		}
		return FReply::Handled();
	}

	return SGraphNode::OnAddPin();
}

void SMDFastBindingGraphNodeWidget::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	SGraphNode::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);

	UpdateDebugTooltip(InCurrentTime);
}

void SMDFastBindingGraphNodeWidget::GetOverlayBrushes(bool bSelected, const FVector2D WidgetSize, TArray<FOverlayBrushInfo>& Brushes) const
{
	if (const UMDFastBindingObject* BindingObject = GetBindingObject())
	{
		FOverlayBrushInfo UpdateTypeBrush;

		if (const ISlateStyle* FastBindingStyle = FSlateStyleRegistry::FindSlateStyle(TEXT("MDFastBindingEditorStyle")))
		{
			switch (BindingObject->GetUpdateType())
			{
			case EMDFastBindingUpdateType::Once:
				UpdateTypeBrush.Brush = FastBindingStyle->GetBrush(TEXT("Icon.UpdateType.Once"));
				break;
			case EMDFastBindingUpdateType::EventBased:
				UpdateTypeBrush.Brush = FastBindingStyle->GetBrush(TEXT("Icon.UpdateType.EventBased"));
				break;
			case EMDFastBindingUpdateType::IfUpdatesNeeded:
				UpdateTypeBrush.Brush = FastBindingStyle->GetBrush(TEXT("Icon.UpdateType.IfUpdatesNeeded"));
				break;
			case EMDFastBindingUpdateType::Always:
				UpdateTypeBrush.Brush = FastBindingStyle->GetBrush(TEXT("Icon.UpdateType.Always"));
				break;
			}
		}

		if (UpdateTypeBrush.Brush != nullptr)
		{
			UpdateTypeBrush.OverlayOffset.X = (WidgetSize.X - (UpdateTypeBrush.Brush->ImageSize.X / 2.f)) - 3.f;
			UpdateTypeBrush.OverlayOffset.Y = (UpdateTypeBrush.Brush->ImageSize.Y / -2.f) + 2.f;
		}

		Brushes.Add(UpdateTypeBrush);
	}
}

bool SMDFastBindingGraphNodeWidget::IsSelfPin(UEdGraphPin& Pin) const
{
	if (const UMDFastBindingObject* BindingObject = GetBindingObject())
	{
		return BindingObject->DoesBindingItemDefaultToSelf(Pin.GetFName());
	}

	return false;
}

void SMDFastBindingGraphNodeWidget::UpdateDebugTooltip(double InCurrentTime)
{
	auto IsTooltipHovered = [&]()
	{
		if (PinValueInspectorPtr.IsValid())
		{
			// The parent of the inspector is the actual tooltip
			const TSharedPtr<SWidget> ParentWidget = PinValueInspectorPtr.Pin()->GetParentWidget();
			return ParentWidget.IsValid() && ParentWidget->IsHovered();
		}

		return false;
	};

	if (IsTooltipHovered())
	{
		// Don't update while interacting with the tooltip
		UnhoveredTooltipCloseTime = InCurrentTime + CloseTooltipGracePeriod;
		return;
	}

	if (UMDFastBindingObject* DebugObject = GetBindingObjectBeingDebugged())
	{
		for (const TSharedRef<SGraphPin>& Pin : InputPins)
		{
			if (Pin->IsHovered())
			{
				SetDebugTooltipPin(Pin->GetCachedGeometry(), Pin->GetPinObj(), DebugObject);
				return;
			}
		}
	}

	// Output pins need to redirect to their connected input since that's where the values are stored
	for (const TSharedRef<SGraphPin>& Pin : OutputPins)
	{
		if (Pin->IsHovered())
		{
			if (UEdGraphPin* GraphPin = Pin->GetPinObj())
			{
				if (GraphPin->LinkedTo.Num() > 0)
				{
					// Binding pins only have a single connection
					if (UEdGraphPin* LinkedPin = GraphPin->LinkedTo[0])
					{
						if (const UMDFastBindingGraphNode* LinkedNode = Cast<UMDFastBindingGraphNode>(LinkedPin->GetOwningNode()))
						{
							if (UMDFastBindingObject* DebugObject = LinkedNode->GetBindingObjectBeingDebugged())
							{
								SetDebugTooltipPin(Pin->GetCachedGeometry(), LinkedPin, DebugObject);
								return;
							}
						}
					}
				}
			}
		}
	}

	if (PinValueInspectorTooltip.IsValid())
	{
		// Close the tooltip after the grace period
		if (FMath::IsNearlyZero(UnhoveredTooltipCloseTime))
		{
			UnhoveredTooltipCloseTime = InCurrentTime + CloseTooltipGracePeriod;
		}
		else if (InCurrentTime >= UnhoveredTooltipCloseTime)
		{
			ClearPinDebugTooltip();
		}
	}
}

void SMDFastBindingGraphNodeWidget::SetDebugTooltipPin(const FGeometry& PinGeometry, UEdGraphPin* Pin, UMDFastBindingObject* DebugObject)
{
	UnhoveredTooltipCloseTime = 0.0;
	if (!PinValueInspectorTooltip.IsValid() && DebugObject)
	{
		const TSharedRef<SMDFastBindingPinValueInspector> PinValueInspector = SNew(SMDFastBindingPinValueInspector);
		PinValueInspector->SetReferences(Pin, DebugObject);
		PinValueInspectorPtr = PinValueInspector;
		PinValueInspectorTooltip = FPinValueInspectorTooltip::SummonTooltip(Pin, PinValueInspector);

		const TSharedPtr<FPinValueInspectorTooltip> ValueTooltip = PinValueInspectorTooltip.Pin();
		if (ValueTooltip.IsValid())
		{
			// Calculate the offset so that the tooltip appears near the pin vertically
			FVector2D PinOffset = FVector2D(PinGeometry.AbsolutePosition) / PinGeometry.Scale - GetUnscaledPosition();
			PinOffset.X = 5.f;
			PinOffset.Y += PinGeometry.Size.Y * 0.5f;

			FVector2D TooltipLocation;
			CalculatePinTooltipLocation(PinOffset, TooltipLocation);
			ValueTooltip->MoveTooltip(TooltipLocation);
		}
	}
	else if (!DebugObject || (PinValueInspectorPtr.IsValid() && !PinValueInspectorPtr.Pin()->Matches(Pin, DebugObject)))
	{
		ClearPinDebugTooltip();
	}
}

void SMDFastBindingGraphNodeWidget::ClearPinDebugTooltip()
{
	if (PinValueInspectorTooltip.IsValid())
	{
		PinValueInspectorTooltip.Pin()->TryDismissTooltip();
	}
}

void SMDFastBindingGraphNodeWidget::CalculatePinTooltipLocation(const FVector2D& Offset, FVector2D& OutTooltipLocation)
{
	TSharedPtr<SGraphPanel> GraphPanel = GetOwnerPanel();
	if (GraphPanel.IsValid())
	{
		// Reset to the pin's location in graph space.
		OutTooltipLocation = GetPosition() + Offset;

		// Shift the desired location to the right edge of the pin's geometry.
		OutTooltipLocation.X += GetTickSpaceGeometry().Size.X;

		// Align to the first entry in the inspector's tree view.
		TSharedPtr<FPinValueInspectorTooltip> Inspector = PinValueInspectorTooltip.Pin();
		if (Inspector.IsValid() && Inspector->ValueInspectorWidget.IsValid())
		{
			// Some hacky magic numbers for ease
			static const float VerticalOffsetWithSearchFilter = 41.0f;
			static const float VerticalOffsetWithoutSearchFilter = 19.0f;

			if (Inspector->ValueInspectorWidget->ShouldShowSearchFilter())
			{
				OutTooltipLocation.Y -= VerticalOffsetWithSearchFilter;
			}
			else
			{
				OutTooltipLocation.Y -= VerticalOffsetWithoutSearchFilter;
			}
		}

		// Convert our desired location from graph coordinates into panel space.
		OutTooltipLocation -= GraphPanel->GetViewOffset();
		OutTooltipLocation *= GraphPanel->GetZoomAmount();

		// Finally, convert the modified location from panel space into screen space.
		OutTooltipLocation = GraphPanel->GetTickSpaceGeometry().LocalToAbsolute(OutTooltipLocation);
	}
}

#undef LOCTEXT_NAMESPACE
