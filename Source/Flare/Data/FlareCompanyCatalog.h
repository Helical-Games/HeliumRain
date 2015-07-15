#pragma once

#include "../Game/FlareCompany.h"
#include "FlareCompanyCatalog.generated.h"


UCLASS()
class UFlareCompanyCatalog : public UDataAsset
{
	GENERATED_UCLASS_BODY()

public:

	/*----------------------------------------------------
		Public data
	----------------------------------------------------*/
	
	/** Company data */
	UPROPERTY(EditAnywhere, Category = Content)
	TArray<FFlareCompanyInfo> Companies;

};