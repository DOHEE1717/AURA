// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AuraCenterAimTargetActor.generated.h"

class UDecalComponent;

/**
 * 화면 중앙(카메라 기준) 레이로 위치를 갱신하는
 * 공용 TargetActor 베이스
 */
UCLASS()
class AURA_API AAuraCenterAimTargetActor : public AActor
{
	GENERATED_BODY()

public:
	AAuraCenterAimTargetActor();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;

protected:
	// ===== Components =====
	// BP에서 데칼을 붙여도 되고, 여기 걸어둔 데칼을 써도 됨
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Targeting")
	TObjectPtr<UDecalComponent> TargetDecal;

protected:
	// ===== Center Ray Settings =====
	// 프리뷰 최대 거리
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Targeting")
	float MaxTraceDistance = 100000.f;
	
protected:
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Targeting")
	FVector CurrentTargetLocation = FVector::ZeroVector;

public:
	UFUNCTION(BlueprintCallable, Category="Targeting")
	FVector GetCurrentTargetLocation() const { return CurrentTargetLocation; }

	// // 지면만 허용할지 여부 (false면 벽/오브젝트도 허용)
	// UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Targeting")
	// bool bRequireGroundHit = false;
	//
	// // 지면 판정 채널 (bRequireGroundHit=true일 때 사용)
	// UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Targeting")
	// TEnumAsByte<ECollisionChannel> GroundTraceChannel = ECC_Visibility;

protected:
	// 중앙 레이로 위치 갱신
	void UpdateLocationFromViewCenter();

	// Ignore Actors 구성
	void BuildIgnoreActors(TArray<AActor*>& OutIgnore) const;
};
