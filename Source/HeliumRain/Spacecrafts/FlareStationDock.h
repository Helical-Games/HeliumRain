#pragma once

#include "../Spacecrafts/FlareSpacecraftComponent.h"
#include "FlareStationDock.generated.h"


UCLASS(Blueprintable, ClassGroup = (Flare, Ship), meta = (BlueprintSpawnableComponent))
class UFlareStationDock : public UFlareSpacecraftComponent
{
public:

	GENERATED_UCLASS_BODY()

	/** Dock size */
	UPROPERTY(EditAnywhere, Category = Content) TEnumAsByte<EFlarePartSize::Type> DockSize;
};
