// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OrbitalReconActor.generated.h"


class UCameraComponent;
class USpringArmComponent;

UCLASS()
class AURA_API AOrbitalReconActor : public AActor
{
	GENERATED_BODY()
	
public:	
	
	AOrbitalReconActor();

	// ReconComponent가 스폰 직후 호출해서 "어디를 보여줄지" 설정
	void InitView(const FVector& InCenter, float InRadius);
	
protected:
	

	// PC가 이 Actor를 ViewTarget으로 잡을 때 쓰기 편하게 제공
	FORCEINLINE UCameraComponent* GetViewCamera() const { return Camera; }

protected:
	virtual void BeginPlay() override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Recon")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Recon")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Recon")
	TObjectPtr<UCameraComponent> Camera;

	// 현재 스캔 중심/반경 (HUD에서 표기용으로도 사용 가능)
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Recon")
	FVector ScanCenter = FVector::ZeroVector;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category="Recon")
	float ScanRadius = 0.f;

	// 카메라 기본 세팅(상공 높이/각도)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Recon|Tuning")
	float DefaultArmLength = 2200.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Recon|Tuning")
	FRotator DefaultArmRotation = FRotator(-75.f, 0.f, 0.f);

	// 스캔 영역이 화면 중앙에 오도록 약간 위로 올리는 보정
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Recon|Tuning")
	float CenterZOffset = 300.f;

};
