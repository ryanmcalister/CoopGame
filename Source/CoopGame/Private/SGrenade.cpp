// Fill out your copyright notice in the Description page of Project Settings.

#include "Public/SGrenade.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "SWeapon.h"

// Sets default values
ASGrenade::ASGrenade()
{
	// Use a sphere as a simple collision representation
	CollisionComp = CreateDefaultSubobject<USphereComponent>(TEXT("SphereComp"));
	CollisionComp->InitSphereRadius(5.0f);
	CollisionComp->SetCollisionProfileName("Projectile");
	
	// Players can't walk on it
	CollisionComp->SetWalkableSlopeOverride(FWalkableSlopeOverride(WalkableSlope_Unwalkable, 0.f));
	CollisionComp->CanCharacterStepUpOn = ECB_No;

	// Set as root component
	RootComponent = CollisionComp;

	// Use a ProjectileMovementComponent to govern this projectile's movement
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileComp"));
	ProjectileMovement->UpdatedComponent = CollisionComp;
	ProjectileMovement->InitialSpeed = 1000.f;
	ProjectileMovement->MaxSpeed = 1000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = true;

	// Die after 1 seconds by default
	InitialLifeSpan = 2.0f;

	SetReplicates(true);
	SetReplicateMovement(true);
}

void ASGrenade::BeginPlay()
{
	Super::BeginPlay();

	GetWorld()->GetTimerManager().SetTimer(ExplodeTimerHandle, this, &ASGrenade::OnExplode, 1.0f, false);

}

void ASGrenade::OnExplode()
{
	auto World = GetWorld();
	if (!World) return;
	auto Location = GetActorLocation();
	auto Weapon = Cast<ASWeapon>(GetOwner());
	
	if (Weapon) {
		TArray<AActor*> ignored;
		ignored.Push(this);
		ignored.Push(Weapon->GetOwner());
		UGameplayStatics::ApplyRadialDamage(World, 100.0f, Location, 100.0f, Weapon->DamageType, ignored, this, GetInstigatorController(), true);
	}

	// play an explosion effect
	if (ExplosionEffect) {
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ExplosionEffect, GetActorLocation(), GetActorRotation());
	}

	Destroy();
}



