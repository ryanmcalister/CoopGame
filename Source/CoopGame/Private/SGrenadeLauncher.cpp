// Fill out your copyright notice in the Description page of Project Settings.

#include "SGrenadeLauncher.h"
#include "SGrenade.h"
#include "Kismet/GameplayStatics.h"



void ASGrenadeLauncher::Fire()
{
	AActor* MyOwner = GetOwner();		
	if (ProjectileClass && MyOwner) {
		// spawn a projectile actor
		FVector MuzzleLocation = MeshComp->GetSocketLocation(MuzzleSocketName);
		FRotator MuzzleRotation = MeshComp->GetSocketRotation(MuzzleSocketName);

		FVector EyeLocation;
		FRotator EyeRotation;
		MyOwner->GetActorEyesViewPoint(EyeLocation, EyeRotation);

		//Set Spawn Collision Handling Override
		FActorSpawnParameters ActorSpawnParams;
		ActorSpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
		ActorSpawnParams.Owner = this;
		ActorSpawnParams.Instigator = MyOwner->GetInstigator();

		// spawn the projectile at the muzzle
		GetWorld()->SpawnActor<ASGrenade>(ProjectileClass, MuzzleLocation, EyeRotation, ActorSpawnParams);
	}

	if (MuzzleEffect)
	{
		UGameplayStatics::SpawnEmitterAttached(MuzzleEffect, MeshComp, MuzzleSocketName);
	}
}
