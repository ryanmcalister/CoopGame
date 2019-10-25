// Fill out your copyright notice in the Description page of Project Settings.

#include "SExplodingBarrel.h"
#include "Components/MeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SHealthComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "DrawDebugHelpers.h"
#include "Net/UnrealNetwork.h"


// Sets default values
ASExplodingBarrel::ASExplodingBarrel()
{
	CollisionComp = CreateDefaultSubobject<UCapsuleComponent>(TEXT("SphereComp"));
	CollisionComp->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComp->SetSimulatePhysics(true);
	RootComponent = CollisionComp;

	MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
	MeshComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MeshComp->SetupAttachment(CollisionComp);

	HealthComp = CreateDefaultSubobject<USHealthComponent>(TEXT("HealthComp"));

	bExploded = false;

	SetReplicates(true);

	BlastRadius = 200.f;
}

// Called when the game starts or when spawned
void ASExplodingBarrel::BeginPlay()
{
	Super::BeginPlay();


	HealthComp->OnHealthChanged.AddDynamic(this, &ASExplodingBarrel::OnHealthChanged);
	
}

void ASExplodingBarrel::OnHealthChanged(USHealthComponent* OwningHealthComp, float Health, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser)
{
	if (Health <= 0.0f && !bExploded)
	{

		auto World = GetWorld();
		if (!World) return;
		auto Location = GetActorLocation();

		// Explode!!
		bExploded = true;

		Explode();

		// radial force damage
		TArray<AActor*> ignored;
		ignored.Push(this);
		UGameplayStatics::ApplyRadialDamage(World, 100.0f, Location, BlastRadius, BarrelDamageType, ignored, this, DamageCauser->GetInstigatorController(), true);
		
		
		// apply radial impulse
		TArray<AActor*> hits;
		TArray<TEnumAsByte<EObjectTypeQuery> > objTypes;
		objTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_PhysicsBody));
		objTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldDynamic));
		objTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldStatic));
		objTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_Pawn));
		UKismetSystemLibrary::SphereOverlapActors(World, Location, BlastRadius, objTypes, nullptr, ignored, hits);
		for (auto &actor : hits) {
			
			auto comp = actor->GetRootComponent();
			if (!comp->IsSimulatingPhysics()) {
				continue;
			}
			auto prim = Cast<UPrimitiveComponent>(comp);
			if (prim) {
				prim->AddRadialImpulse(Location, BlastRadius, 500.0, ERadialImpulseFalloff::RIF_Constant, true);
			}
		}

	}
}

void ASExplodingBarrel::Explode()
{
	auto World = GetWorld();
	if (!World) return;
	auto Location = GetActorLocation();

	// change material
	Exploded();

	// particle effect
	// play an explosion effect
	if (ExplosionEffect) {
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation(), GetActorRotation());
	}

	// launch barrel upwards
	CollisionComp->AddImpulse(FVector(0, 0, 40000.0f));

	DrawDebugSphere(World, Location, BlastRadius, 16, FColor::Cyan, true, 3);

}

void ASExplodingBarrel::OnRep_Exploded()
{
	if (bExploded)
	{
		Explode();

	}
}

void ASExplodingBarrel::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ASExplodingBarrel, bExploded);
}

