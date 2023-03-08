#include "Customizations/MDFastBindingObjectCustomization.h"

#include "DetailLayoutBuilder.h"
#include "MDFastBindingObject.h"

void FMDFastBindingObjectCustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	DetailBuilder.GetDetailsView()->SetIsPropertyReadOnlyDelegate(FIsPropertyReadOnly::CreateSP(this, &FMDFastBindingObjectCustomization::IsPropertyReadOnly));
}

void FMDFastBindingObjectCustomization::CustomizeDetails(const TSharedPtr<IDetailLayoutBuilder>& DetailBuilder)
{
	WeakDetailBuilder = DetailBuilder;
	IDetailCustomization::CustomizeDetails(DetailBuilder);
}

bool FMDFastBindingObjectCustomization::IsPropertyReadOnly(const FPropertyAndParent& PropertyAndParent) const
{
	if (PropertyAndParent.Property.GetFName() == TEXT("UpdateType"))
	{
		if (const TSharedPtr<IDetailLayoutBuilder> DetailBuilder = WeakDetailBuilder.Pin())
		{
			TArray<TWeakObjectPtr<UObject>> Objects;
			DetailBuilder->GetObjectsBeingCustomized(Objects);

			for (TWeakObjectPtr<UObject> Object : Objects)
			{
				if (const UMDFastBindingObject* BindingObject = Cast<UMDFastBindingObject>(Object.Get()))
				{
					// Cannot edit EventBased objects update type
					if (BindingObject->GetUpdateType() == EMDFastBindingUpdateType::EventBased)
					{
						return true;;
					}

				}
			}
		}
	}

	return false;
}
