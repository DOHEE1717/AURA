#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PhaseShiftRecallComponent.generated.h"

UCLASS(ClassGroup=(Aura), meta=(BlueprintSpawnableComponent))
class AURA_API UPhaseShiftRecallComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPhaseShiftRecallComponent();

	/** 저장된 위치가 유효한가 */
	UFUNCTION(BlueprintCallable, Category="PhaseShift")
	bool HasSavedLocation() const;

	/** 현재 위치 저장 (월드 위치/회전) */
	UFUNCTION(BlueprintCallable, Category="PhaseShift")
	void SaveCurrentLocation(const AActor* ActorToReadFrom);

	/** 저장된 위치로 텔레포트 */
	UFUNCTION(BlueprintCallable, Category="PhaseShift")
	bool RecallToSavedLocation(AActor* ActorToMove, bool bClearAfterRecall = true);

	/** 저장값 제거 */
	UFUNCTION(BlueprintCallable, Category="PhaseShift")
	void ClearSavedLocation();

	/** 타임아웃 설정(선택) */
	UFUNCTION(BlueprintCallable, Category="PhaseShift")
	void SetExpireSeconds(float InSeconds);

	/** 만료 체크(선택) */
	UFUNCTION(BlueprintCallable, Category="PhaseShift")
	bool IsExpired() const;

private:
	UPROPERTY()
	bool bHasSaved = false;

	UPROPERTY()
	FVector SavedLocation = FVector::ZeroVector;

	UPROPERTY()
	FRotator SavedRotation = FRotator::ZeroRotator;

	// 저장 시각
	UPROPERTY()
	float SavedWorldTime = 0.f;

	// 0 이하면 만료 없음
	UPROPERTY()
	float ExpireSeconds = 0.f;
};
