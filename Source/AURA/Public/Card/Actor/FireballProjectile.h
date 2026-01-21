// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "FireballProjectile.generated.h"

class USphereComponent;
class UProjectileMovementComponent;

UCLASS()
class AURA_API AFireballProjectile : public AActor
{
	GENERATED_BODY()

public:
	AFireballProjectile();

	// Ability에서 호출: 방향/속도 세팅
	UFUNCTION(BlueprintCallable, Category="Projectile")
	void FireInDirection(const FVector& Direction, float Speed);

protected:
	virtual void BeginPlay() override;

protected:
	UPROPERTY(VisibleAnywhere, Category="Components")
	USphereComponent* Collision;

	UPROPERTY(VisibleAnywhere, Category="Components")
	UProjectileMovementComponent* ProjectileMovement;
};
