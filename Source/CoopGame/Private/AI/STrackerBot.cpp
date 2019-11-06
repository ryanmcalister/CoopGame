// Fill out your copyright notice in the Description page of Project Settings.

#include "AI/STrackerBot.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "GameFramework/Character.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "DrawDebugHelpers.h"
#include "Components/SHealthComponent.h"
#include "SCharacter.h"
#include "Sound/SoundCue.h"
#include "Net/UnrealNetwork.h"
#include "Math/UnrealMathUtility.h"


// Sets default values
ASTrackerBot::ASTrackerBot()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetSimulatePhysics(true);
	MeshComp->SetCanEverAffectNavigation(false);
	RootComponent = MeshComp;

	HealthComp = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComp"));
	HealthComp->OnHealthChanged.AddDynamic(this, &ASTrackerBot::HandleTakeDamage);

	SphereComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	SphereComp->SetSphereRadius(200);
	SphereComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	SphereComp->SetCollisionResponseToAllChannels(ECR_Ignore);
	SphereComp->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	SphereComp->SetupAttachment(RootComponent);
	
	bUseVelocityChange = false;
	bStartedSelfDestruction = false;
	MovementForce = 1000.f;
	RequiredDistanceToTarget = 100.f;
	bExploded = false;
	ExplosionDamage = 40.f;
	ExplosionRadius = 200.f;
	SelfDamageInterval = 0.25f;
	MaxPowerLevel = 100.f;
	PowerLevel = 0.f;
	SetReplicates(true);
	PowerRadius = 200.0f;
	PowerBonus = 20.0f;


}

// Called when the game starts or when spawned
void ASTrackerBot::BeginPlay()
{
	Super::BeginPlay();
	
	if (Role == ROLE_Authority)
	{
		NextPathPoint = GetNextPathPoint();
		PowerLevel = 0.f;
		GetWorldTimerManager().SetTimer(TimerHandle_UpdatePowerLevel, this, &ASTrackerBot::UpdatePowerLevel, 1.0f, true, 1.0f);
	}
}

void ASTrackerBot::NotifyActorBeginOverlap(AActor* OtherActor)
{
	if (!bStartedSelfDestruction && !bExploded)
	{
		ASCharacter* PlayerPawn = Cast<ASCharacter>(OtherActor);
		if (PlayerPawn)
		{
			if (Role == ROLE_Authority)
			{
				// start self destruction sequence
				GetWorldTimerManager().SetTimer(TimerHandle_SelfDamage, this, &ASTrackerBot::DamageSelf, SelfDamageInterval, true, 0.0f);

				bStartedSelfDestruction = true;

				UGameplayStatics::SpawnSoundAttached(SelfDestructSound, RootComponent);
			}
		}
	}
	
}

void ASTrackerBot::DamageSelf()
{
	UGameplayStatics::ApplyDamage(this, 20.0, GetInstigatorController(), this, nullptr);
}

void ASTrackerBot::UpdatePowerLevel()
{
	auto World = GetWorld();
	if (!World) return;

	auto Location = GetActorLocation();
	// apply radial impulse
	TArray<AActor*> hits;
	TArray<AActor*> ignored;
	ignored.Push(this);
	TArray<TEnumAsByte<EObjectTypeQuery> > objTypes;
	objTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));
	UKismetSystemLibrary::SphereOverlapActors(World, Location, PowerRadius, objTypes, nullptr, ignored, hits);
	int botCount = 0;
	for (auto &actor : hits) {
		auto bot = Cast<ASTrackerBot>(actor);
		if (bot)
		{
			botCount++;
		}
	}
	PowerLevel = FMath::Clamp<float>(botCount * PowerBonus, 0, MaxPowerLevel);
	OnRep_PowerLevel();
}

FVector ASTrackerBot::GetNextPathPoint()
{
	// Hack to get player location
	ACharacter* PlayerPawn = UGameplayStatics::GetPlayerCharacter(this, 0);

	UNavigationPath* NavPath = UNavigationSystemV1::FindPathToActorSynchronously(this, GetActorLocation(), PlayerPawn);
	if (NavPath)
	{
		if (NavPath->PathPoints.Num() > 1)
		{
			// Return next point in the path
			return NavPath->PathPoints[1];
		}
	}

	// Failed to find path
	return GetActorLocation();
}

// Called every frame
void ASTrackerBot::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (Role == ROLE_Authority && !bExploded)
	{

		float DistanceToTarget = (GetActorLocation() - NextPathPoint).Size();

		if (DistanceToTarget <= RequiredDistanceToTarget)
		{
			NextPathPoint = GetNextPathPoint();

			DrawDebugString(GetWorld(), GetActorLocation(), "Target Reached!", (AActor *)0, FColor::Green, 0);
		}
		else
		{
			FVector ForceDirection = NextPathPoint - GetActorLocation();
			ForceDirection.Normalize();

			ForceDirection *= MovementForce;

			// Keep moving towards next target
			MeshComp->AddForce(ForceDirection, NAME_None, bUseVelocityChange);

			DrawDebugDirectionalArrow(GetWorld(), GetActorLocation(), GetActorLocation() + ForceDirection, 32, FColor::Yellow, false, 0.0f, 0, 1.0f);
		}

		DrawDebugSphere(GetWorld(), NextPathPoint, 20, 12, FColor::Yellow, false, 0.0f, 1.0f);
	}
}

void ASTrackerBot::SelfDestruct()
{
	if (bExploded)
	{
		return;
	}
	bExploded = true;

	UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation());

	UGameplayStatics::PlaySoundAtLocation(this, ExplodeSound, GetActorLocation());

	MeshComp->SetVisibility(false, true);
	MeshComp->SetSimulatePhysics(false);
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	if (Role == ROLE_Authority)
	{
		TArray<AActor*> IgnoreActors;
		IgnoreActors.Add(this);

		// Apply radial damage
		UGameplayStatics::ApplyRadialDamage(this, ExplosionDamage + PowerLevel, GetActorLocation(), ExplosionRadius, nullptr, IgnoreActors, this, GetInstigatorController(), true);


		DrawDebugSphere(GetWorld(), GetActorLocation(), ExplosionRadius, 12, FColor::Red, false, 2.0f, 0, 1.0f);

		// Delete actor
		SetLifeSpan(2.0f);
	}
}


void ASTrackerBot::HandleTakeDamage(USHealthComponent* OwningHealthComp, float Health, float HealthDelta, const class UDamageType* DamageType, 
	class AController* InstigatedBy, AActor* DamageCauser)
{
	if (MatInst == nullptr)
	{
		MatInst = MeshComp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, MeshComp->GetMaterial(0));
	}

	if (MatInst)
	{
		MatInst->SetScalarParameterValue("LastTimeDamageTaken", GetWorld()->TimeSeconds);
	}

	UE_LOG(LogTemp, Log, TEXT("Health %s of %s"), *FString::SanitizeFloat(Health), *GetName());

	// Explode on hitpoints == 0
	if (Health <= 0.0f)
	{
		SelfDestruct();
	}
}

void ASTrackerBot::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASTrackerBot, PowerLevel);
}

void ASTrackerBot::OnRep_PowerLevel()
{
	if (MatInst == nullptr)
	{
		MatInst = MeshComp->CreateAndSetMaterialInstanceDynamicFromMaterial(0, MeshComp->GetMaterial(0));
	}

	if (MatInst)
	{
		MatInst->SetScalarParameterValue("PowerLevelAlpha", PowerLevel / MaxPowerLevel);
	}
}


