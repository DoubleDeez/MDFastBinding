#include "Graph/MDFastBindingConnectionDrawingPolicy.h"

#include "BindingDestinations/MDFastBindingDestinationBase.h"
#include "Graph/MDFastBindingGraph.h"
#include "Graph/MDFastBindingGraphNode.h"
#include "MDFastBindingInstance.h"
#include "MDFastBindingObject.h"
#include "Misc/App.h"

FMDFastBindingConnectionDrawingPolicy::FMDFastBindingConnectionDrawingPolicy(int32 InBackLayerID, int32 InFrontLayerID, float ZoomFactor, const FSlateRect& InClippingRect, FSlateWindowElementList& InDrawElements, UEdGraph* InGraphObj)
	: FKismetConnectionDrawingPolicy(InBackLayerID, InFrontLayerID, ZoomFactor, InClippingRect, InDrawElements, InGraphObj)
{
	// We use colored wires so don't tint them on execution
	AttackColor = FLinearColor::White;
}
	
void FMDFastBindingConnectionDrawingPolicy::DetermineWiringStyle(UEdGraphPin* OutputPin, UEdGraphPin* InputPin, /*inout*/ FConnectionParams& Params)
{
	FKismetConnectionDrawingPolicy::DetermineWiringStyle(OutputPin, InputPin, Params);

	if (CanBuildRoadmap())
	{
		UEdGraphNode* OutputNode = OutputPin->GetOwningNode();
		UEdGraphNode* InputNode = (InputPin != nullptr) ? InputPin->GetOwningNode() : nullptr;
		if (FExecPairingMap* ExecMap = PredecessorPins.Find(InputNode))
		{
			if (FTimePair* PinTime = ExecMap->Find(InputPin))
			{
				DetermineStyleOfExecWire(Params.WireThickness, Params.WireColor, Params.bDrawBubbles, *PinTime);
			}
		}
	}
}

void FMDFastBindingConnectionDrawingPolicy::BuildExecutionRoadmap()
{
	if (const UMDFastBindingGraph* Graph = Cast<UMDFastBindingGraph>(GraphObj))
	{
		const UMDFastBindingInstance* BindingBeginDebugged = Graph->GetBindingBeingDebugged();
		const UMDFastBindingInstance* BindingCDO = Graph->GetBinding();
		if (BindingBeginDebugged != nullptr)
		{
			TArray<UMDFastBindingObject*> ObjectsToProcess = { BindingCDO->GetBindingDestination() };
			while (ObjectsToProcess.Num() > 0)
			{
				UMDFastBindingObject* BindingObject = ObjectsToProcess.Pop();
				UMDFastBindingGraphNode* Node = Graph->FindNodeWithBindingObject(BindingObject);
				if (Node == nullptr)
				{
					continue;
				}
				
				UMDFastBindingObject* BindingObjectBeingDebugged = BindingBeginDebugged->FindBindingObjectWithGUID(BindingObject->BindingObjectIdentifier);
				if (BindingObjectBeingDebugged == nullptr)
				{
					continue;
				}
				
				FExecPairingMap& ExecMap = PredecessorPins.FindOrAdd(Node);
				for (UEdGraphPin* Pin : Node->Pins)
				{
					if (Pin != nullptr && Pin->Direction == EGPD_Input)
					{
						if (const FMDFastBindingItem* BindingItem = BindingObjectBeingDebugged->FindBindingItem(Pin->PinName))
						{
							ExecMap.FindOrAdd(Pin).ThisExecTime = BindingItem->LastUpdateTime;
						}

						if (Pin->LinkedTo.Num() > 0 && Pin->LinkedTo[0] != nullptr)
						{
							if (const UMDFastBindingGraphNode* LinkedNode = Cast<UMDFastBindingGraphNode>(Pin->LinkedTo[0]->GetOwningNode()))
							{
								ObjectsToProcess.Push(LinkedNode->GetBindingObject());
							}
						}
					}
				}
			}
		}
	}
	
	const double MaxTimeAhead = FMath::Min(FApp::GetCurrentTime() + 2*TracePositionBonusPeriod, LatestTimeDiscovered);
	CurrentTime = FMath::Max(FApp::GetCurrentTime(), MaxTimeAhead);
}

bool FMDFastBindingConnectionDrawingPolicy::CanBuildRoadmap() const
{
	const UMDFastBindingGraph* Graph = Cast<UMDFastBindingGraph>(GraphObj);
	return Graph != nullptr && Graph->GetBindingBeingDebugged() != nullptr;
}
