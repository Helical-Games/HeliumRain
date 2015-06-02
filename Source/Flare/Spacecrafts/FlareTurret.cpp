
#include "../Flare.h"
#include "FlareTurret.h"
#include "FlareSpacecraft.h"
#include "FlareShell.h"


/*----------------------------------------------------
	Constructor
----------------------------------------------------*/

UFlareTurret::UFlareTurret(const class FObjectInitializer& PCIP)
	: Super(PCIP)
	, TurretComponent(NULL)
	, BarrelComponent(NULL)
{
	HasFlickeringLights = false;
}


/*----------------------------------------------------
	Gameplay
----------------------------------------------------*/

void UFlareTurret::Initialize(const FFlareSpacecraftComponentSave* Data, UFlareCompany* Company, AFlareSpacecraftPawn* OwnerShip, bool IsInMenu)
{
	Super::Initialize(Data, Company, OwnerShip, IsInMenu);
	AimDirection = FVector::ZeroVector;

	// Initialize pilot
	Pilot = NewObject<UFlareTurretPilot>(this, UFlareTurretPilot::StaticClass());
	Pilot->Initialize(&(Data->Pilot), Company, this);
}


void UFlareTurret::SetupComponentMesh()
{
	Super::SetupComponentMesh();

	// Turret Mesh
	if (Spacecraft && ComponentDescription && ComponentDescription->TurretCharacteristics.TurretMesh)
	{

		TurretComponent = NewObject<UFlareSpacecraftSubComponent>(this, UFlareSpacecraftSubComponent::StaticClass(), TEXT("TurretMesh"));
		 if(TurretComponent)
		 {
			TurretComponent->SetParentSpacecraftComponent(this);
			TurretComponent->RegisterComponent();
			TurretComponent->AttachTo(this);
			TurretComponent->SetStaticMesh(ComponentDescription->TurretCharacteristics.TurretMesh);
			TurretComponent->SetMaterial(0, ComponentDescription->TurretCharacteristics.TurretMesh->GetMaterial(0));
			TurretComponent->Initialize(NULL, PlayerCompany, Spacecraft, false);
			Spacecraft->AddOwnedComponent(TurretComponent);
		}
	}

	// Barrel Mesh
	if (Spacecraft && ComponentDescription && ComponentDescription->TurretCharacteristics.BarrelsMesh)
	{

		BarrelComponent = NewObject<UFlareSpacecraftSubComponent>(this, UFlareSpacecraftSubComponent::StaticClass() , TEXT("BarrelMesh"));
		 if (BarrelComponent)
		 {
			 BarrelComponent->SetParentSpacecraftComponent(this);
			BarrelComponent->RegisterComponent();
			if (TurretComponent)
			{
				BarrelComponent->AttachTo(TurretComponent, FName("Axis"));
			}
			else
			{
				BarrelComponent->AttachTo(this);
			}
			BarrelComponent->SetStaticMesh(ComponentDescription->TurretCharacteristics.BarrelsMesh);
			BarrelComponent->SetMaterial(0, ComponentDescription->TurretCharacteristics.BarrelsMesh->GetMaterial(0));
			Spacecraft->AddOwnedComponent(BarrelComponent);
		}
	}
}



void UFlareTurret::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{

	if (Spacecraft->GetDamageSystem()->IsAlive() && Pilot)
	{

		Pilot->TickPilot(DeltaTime);
		//FLOGV("Pilot exist WantFire %d", Pilot->IsWantFire());
		if(Pilot->IsWantFire())
		{
			StartFire();
		}
		else
		{
			StopFire();
		}
		AimDirection = Pilot->GetTargetAimAxis();
		//FLOGV("Pilot AimDirection %s", *AimDirection.ToString());
	}

	if(Spacecraft->GetDamageSystem()->IsAlive() && GetUsableRatio() > 0)
	{

		if(TurretComponent && ComponentDescription)
		{

			float TargetTurretAngle = 0;
			if(AimDirection != FVector::ZeroVector)
			{
				FVector LocalTurretAimDirection = GetComponentToWorld().GetRotation().Inverse().RotateVector(AimDirection);
				TargetTurretAngle = FMath::UnwindDegrees(FMath::RadiansToDegrees(FMath::Atan2(LocalTurretAimDirection.Y, LocalTurretAimDirection.X)));
			}

			// Clamp movements
			TargetTurretAngle = FMath::Clamp(TargetTurretAngle, ComponentDescription->TurretCharacteristics.TurretMinAngle, ComponentDescription->TurretCharacteristics.TurretMaxAngle);

			float UsableTurretVelocity = GetUsableRatio() * ComponentDescription->TurretCharacteristics.TurretAngularVelocity;

			float TurretAngleDiff = FMath::UnwindDegrees(TargetTurretAngle - ShipComponentData.Turret.TurretAngle);

			if(FMath::Abs(TurretAngleDiff) <= UsableTurretVelocity * DeltaTime) {
				ShipComponentData.Turret.TurretAngle = TargetTurretAngle;
			} else if(TurretAngleDiff < 0) {
				ShipComponentData.Turret.TurretAngle -= UsableTurretVelocity * DeltaTime;
			} else {
				ShipComponentData.Turret.TurretAngle += UsableTurretVelocity * DeltaTime;
			}

			TurretComponent->SetRelativeRotation(FRotator(0, ShipComponentData.Turret.TurretAngle, 0));
		}

		if (BarrelComponent)
		{

			float TargetBarrelAngle = 0;

			if(AimDirection != FVector::ZeroVector)
			{
				FVector LocalBarrelAimDirection;
				if (TurretComponent)
				{
					LocalBarrelAimDirection = TurretComponent->GetComponentToWorld().GetRotation().Inverse().RotateVector(AimDirection);
				}
				else
				{
					LocalBarrelAimDirection = GetComponentToWorld().GetRotation().Inverse().RotateVector(AimDirection);
				}

				TargetBarrelAngle = FMath::UnwindDegrees(FMath::RadiansToDegrees(FMath::Atan2(LocalBarrelAimDirection.Z, LocalBarrelAimDirection.X)));
			}

			// Clamp movements
			TargetBarrelAngle = FMath::Clamp(TargetBarrelAngle, GetMinLimitAtAngle(ShipComponentData.Turret.TurretAngle), ComponentDescription->TurretCharacteristics.BarrelsMaxAngle);


			// TODO Add ship specific bound

			float UsableBarrelsVelocity = GetUsableRatio() * ComponentDescription->TurretCharacteristics.TurretAngularVelocity;
			float BarrelAngleDiff = FMath::UnwindDegrees(TargetBarrelAngle - ShipComponentData.Turret.BarrelsAngle);

			if(FMath::Abs(BarrelAngleDiff) <= UsableBarrelsVelocity * DeltaTime) {
				ShipComponentData.Turret.BarrelsAngle = TargetBarrelAngle;
			} else if(BarrelAngleDiff < 0) {
				ShipComponentData.Turret.BarrelsAngle -= UsableBarrelsVelocity * DeltaTime;
			} else {
				ShipComponentData.Turret.BarrelsAngle += UsableBarrelsVelocity * DeltaTime;
			}
			BarrelComponent->SetRelativeRotation(FRotator(ShipComponentData.Turret.BarrelsAngle, 0, 0));

		}
	}

	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

}

FVector UFlareTurret::GetFireAxis() const
{
	if (BarrelComponent)
	{
		return BarrelComponent->GetComponentRotation().RotateVector(FVector(1, 0, 0));
	}
	else if (TurretComponent)
	{
		return TurretComponent->GetComponentRotation().RotateVector(FVector(1, 0, 0));
	}
	else
	{
		return Super::GetFireAxis();
	}
}

FVector UFlareTurret::GetIdleAxis() const
{
	// Ship front
	return Spacecraft->Airframe->GetComponentRotation().RotateVector(FVector(1, 0, 0));
}


FVector UFlareTurret::GetMuzzleLocation(int GunIndex) const
{
	const UStaticMeshComponent* GunComponent = this;
	if (BarrelComponent)
	{
		GunComponent = BarrelComponent;
	}
	else if (TurretComponent)
	{
		GunComponent = TurretComponent;
	}

	if (ComponentDescription->GunCharacteristics.GunCount <= 1)
	{
		return GunComponent->GetSocketLocation(FName("Muzzle"));
	}
	else
	{
		return GunComponent->GetSocketLocation(FName(*(FString("Muzzle") + FString::FromInt(GunIndex))));
	}
}

FVector UFlareTurret::GetTurretBaseLocation() const
{
	if (BarrelComponent)
	{
		return BarrelComponent->GetComponentLocation();
	}
	else if (TurretComponent)
	{
		return TurretComponent->GetComponentLocation();
	}
	return GetComponentLocation();
}

bool UFlareTurret::IsSafeToFire(int GunIndex) const
{
	FVector FiringLocation = GetMuzzleLocation(GunIndex);
	FVector FiringDirection = GetFireAxis();
	FVector TargetLocation = FiringLocation + FiringDirection * 100000;

	FHitResult HitResult(ForceInit);
	if (Trace(FiringLocation, TargetLocation, HitResult))
	{
		if (HitResult.Actor.IsValid() && HitResult.Actor == Spacecraft)
		{
			FLOG("!!!!!!!!!Not safe to fire !");
			return false;
		}
	}
	return true;
}

bool UFlareTurret::Trace(const FVector& Start, const FVector& End, FHitResult& HitOut) const
{
	FCollisionQueryParams TraceParams(FName(TEXT("Shell Trace")), true, NULL);
	TraceParams.bTraceComplex = true;
	//TraceParams.bTraceAsyncScene = true;
	TraceParams.bReturnPhysicalMaterial = false;

	//Re-initialize hit info
	HitOut = FHitResult(ForceInit);

	ECollisionChannel CollisionChannel = (ECollisionChannel) (ECC_WorldStatic | ECC_WorldDynamic | ECC_Pawn);

	//Trace!
	GetWorld()->LineTraceSingleByChannel(
		HitOut,		//result
		Start,	//start
		End , //end
		CollisionChannel, //collision channel
		TraceParams
	);

	//Hit any Actor?
	return (HitOut.GetActor() != NULL) ;
}

bool UFlareTurret::IsReacheableAxis(FVector TargetAxis) const
{
	float TargetTurretAngle = 0;
	if(TurretComponent && ComponentDescription)
	{

		FVector LocalTurretAimDirection = GetComponentToWorld().GetRotation().Inverse().RotateVector(TargetAxis);
		TargetTurretAngle = FMath::UnwindDegrees(FMath::RadiansToDegrees(FMath::Atan2(LocalTurretAimDirection.Y, LocalTurretAimDirection.X)));

		if(TargetTurretAngle > ComponentDescription->TurretCharacteristics.TurretMaxAngle
				|| TargetTurretAngle < ComponentDescription->TurretCharacteristics.TurretMinAngle)
		{
			return false;
		}
	}

	if (BarrelComponent && ComponentDescription)
	{

		FVector LocalBarrelAimDirection;
		if (TurretComponent)
		{
			LocalBarrelAimDirection = TurretComponent->GetComponentToWorld().GetRotation().Inverse().RotateVector(TargetAxis);
		}
		else
		{
			LocalBarrelAimDirection = GetComponentToWorld().GetRotation().Inverse().RotateVector(TargetAxis);
		}

		float TargetBarrelAngle = FMath::UnwindDegrees(FMath::RadiansToDegrees(FMath::Atan2(LocalBarrelAimDirection.Z, LocalBarrelAimDirection.X)));
		if(TargetBarrelAngle > ComponentDescription->TurretCharacteristics.BarrelsMaxAngle
				|| TargetBarrelAngle < GetMinLimitAtAngle(TargetTurretAngle))
		{
			return false;
		}




	}
	return true;
}

static inline int PositiveModulo(int i, int n) {
	return (i % n + n) % n;
}

float UFlareTurret::GetMinLimitAtAngle(float Angle) const
{
	float BarrelsMinAngle = ComponentDescription->TurretCharacteristics.BarrelsMinAngle;

	//Fine Local slot check
	for (int32 i = 0; i < Spacecraft->GetDescription()->TurretSlots.Num(); i++)
	{
		// TODO optimize and store that in cache
		if(Spacecraft->GetDescription()->TurretSlots[i].SlotIdentifier == ShipComponentData.ShipSlotIdentifier)
		{
			int LimitStepCount = Spacecraft->GetDescription()->TurretSlots[i].TurretBarrelsAngleLimit.Num();


			if(LimitStepCount > 0)
			{
				float StepAngle = 360.f / (float) LimitStepCount;


				float AngleInStep = Angle / StepAngle;
				int NearestStep = FMath::FloorToInt(AngleInStep + 0.5f);
				int SecondNearestStep;
				if (AngleInStep > NearestStep)
				{
					SecondNearestStep = NearestStep+1;
				}
				else
				{
					SecondNearestStep = NearestStep-1;
				}

				float Ratio = FMath::Abs(Angle - NearestStep * StepAngle) /  StepAngle;

				float LocalMin = Spacecraft->GetDescription()->TurretSlots[i].TurretBarrelsAngleLimit[PositiveModulo(NearestStep, LimitStepCount)] * (1.f - Ratio)
									+ Spacecraft->GetDescription()->TurretSlots[i].TurretBarrelsAngleLimit[PositiveModulo(SecondNearestStep,LimitStepCount)] * Ratio;

				BarrelsMinAngle = FMath::Max(BarrelsMinAngle, LocalMin);
			}
		}

	}
	return BarrelsMinAngle;
}



void UFlareTurret::GetBoundingSphere(FVector& Location, float& SphereRadius)
{
	Super::GetBoundingSphere(Location, SphereRadius);
	if(TurretComponent || BarrelComponent)
	{
		SphereRadius = 0;
	}
}