// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "SExplodingBarrel.generated.h"

class UStaticMeshComponent;
class UCapsuleComponent;
class USHealthComponent;
class UDamageType;

UCLASS()
class COOPGAME_API ASExplodingBarrel : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	ASExplodingBarrel();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Components")
	UStaticMeshComponent* MeshComp;

	UPROPERTY(VisibleAnywhere, Category = "Components")
	UCapsuleComponent* CollisionComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	USHealthComponent* HealthComp;

	UFUNCTION()
	void OnHealthChanged(USHealthComponent* OwningHealthComp, float Health, float HealthDelta, const class UDamageType* DamageType, class AController* InstigatedBy, AActor* DamageCauser);

	UPROPERTY(BlueprintReadOnly, Category = "Barrel")
	bool bExploded;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barrel")
	UParticleSystem* ExplosionEffect;

	UFUNCTION(BlueprintImplementableEvent, Category = "Barrel")
	void Exploded();

	

public:	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Barrel")
	TSubclassOf<UDamageType> BarrelDamageType;
	
};
