#include "Card/CardAbility/PhaseShift/CA_PhaseShift_Primary.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"

#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"

#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Engine/World.h"
#include "TimerManager.h"
#include "GameBase/AuraAttributeSet.h"

UCA_PhaseShift_Primary::UCA_PhaseShift_Primary()
{
	// 즉시 발동형 버프(타겟팅 없음)
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	MoveSpeedGameplayEffectClass = nullptr;
}

void UCA_PhaseShift_Primary::ActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UE_LOG(LogTemp, Warning, TEXT("[PhaseShift] ActivateAbility ENTER"));

	// 커밋 실패면 종료 (기존 Pri 스타일)
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		UE_LOG(LogTemp, Warning, TEXT("[PhaseShift] Commit FAILED -> EndAbility"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[PhaseShift] Commit OK"));

	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[PhaseShift] ActorInfo/Avatar invalid -> EndAbility"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 1) 이동속도 적용 (GE 우선, 실패 시 폴백)
	const bool bAppliedGE = TryApplyMoveSpeedGE(ActorInfo);
	if (!bAppliedGE)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PhaseShift] MoveSpeed GE not applied (null or failed)."));

		if (bUseCharacterMovementFallback)
		{
			if (!TryApplyCharacterMovementFallback(ActorInfo))
			{
				UE_LOG(LogTemp, Warning, TEXT("[PhaseShift] Fallback MaxWalkSpeed apply FAILED -> EndAbility"));
				EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
				return;
			}

			UE_LOG(LogTemp, Warning, TEXT("[PhaseShift] Fallback MaxWalkSpeed applied."));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[PhaseShift] Fallback disabled -> EndAbility"));
			EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
			return;
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[PhaseShift] MoveSpeed GE applied OK."));
	}

	// 2) 로컬 FOV 적용(시각 효과는 로컬만)
	ApplyLocalFOV(ActorInfo);

	UE_LOG(LogTemp, Warning, TEXT("[PhaseShift] ActivateAbility OK | Avatar=%s"), *GetNameSafe(ActorInfo->AvatarActor.Get()));

	// 디버그: 0.1초마다 이동속도 로그 (World 안전 획득)
	UWorld* World = nullptr;
	if (ActorInfo && ActorInfo->AvatarActor.IsValid())
	{
		World = ActorInfo->AvatarActor->GetWorld();
	}

	if (World)
	{
		World->GetTimerManager().SetTimer(
			DebugSpeedTimerHandle,
			this,
			&ThisClass::DebugLogSpeedTick,
			0.1f,
			true
		);

		UE_LOG(LogTemp, Warning, TEXT("[PhaseShift] DebugTimer Set | Active=%d"),
			World->GetTimerManager().IsTimerActive(DebugSpeedTimerHandle) ? 1 : 0);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[PhaseShift] DebugTimer FAILED | World null"));
	}

	// 3) Duration 후 종료
	WaitDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, BuffDuration);
	if (!WaitDelayTask)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PhaseShift] WaitDelayTask null -> EndAbility"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	WaitDelayTask->OnFinish.AddDynamic(this, &ThisClass::OnBuffFinished);
	WaitDelayTask->ReadyForActivation();

	UE_LOG(LogTemp, Warning, TEXT("[PhaseShift] WaitDelayTask started | Duration=%.2f"), BuffDuration);
}

void UCA_PhaseShift_Primary::OnBuffFinished()
{
	UE_LOG(LogTemp, Warning, TEXT("[PhaseShift] OnBuffFinished -> EndAbility"));
	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

void UCA_PhaseShift_Primary::DebugLogSpeedTick()
{
	if (!CurrentActorInfo)
		return;

	// 1) Avatar가 Character면 그대로 사용
	ACharacter* Ch = Cast<ACharacter>(CurrentActorInfo->AvatarActor.Get());

	// 2) Avatar가 PlayerState 같은 경우 -> PC의 Pawn을 사용
	if (!Ch)
	{
		if (APlayerController* PC = Cast<APlayerController>(CurrentActorInfo->PlayerController.Get()))
		{
			Ch = Cast<ACharacter>(PC->GetPawn());
		}
	}

	// 3) 그래도 없으면 OwnerActor의 Instigator 등에서 추가 폴백
	if (!Ch)
	{
		if (AActor* Owner = CurrentActorInfo->OwnerActor.Get())
		{
			Ch = Cast<ACharacter>(Owner->GetInstigator());
		}
	}

	if (!Ch)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PhaseShift][Dbg] Character not found (Avatar=%s Owner=%s PC=%s)"),
			*GetNameSafe(CurrentActorInfo->AvatarActor.Get()),
			*GetNameSafe(CurrentActorInfo->OwnerActor.Get()),
			*GetNameSafe(CurrentActorInfo->PlayerController.Get()));
		return;
	}

	UCharacterMovementComponent* MoveComp = Ch->GetCharacterMovement();
	if (!MoveComp)
		return;

	const float MaxSpeed = MoveComp->MaxWalkSpeed;
	const float VelocitySize = Ch->GetVelocity().Size();

	UE_LOG(LogTemp, Log,
		TEXT("[PhaseShift][Dbg] MaxWalkSpeed=%.1f | Velocity=%.1f | Pawn=%s | Avatar=%s"),
		MaxSpeed,
		VelocitySize,
		*GetNameSafe(Ch),
		*GetNameSafe(CurrentActorInfo->AvatarActor.Get())
	);
}

void UCA_PhaseShift_Primary::EndAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	UE_LOG(LogTemp, Warning, TEXT("[PhaseShift] EndAbility ENTER | Cancelled=%d"), bWasCancelled ? 1 : 0);

	// 디버그 타이머 정리 (World 안전 획득)
	UWorld* World = nullptr;
	if (ActorInfo && ActorInfo->AvatarActor.IsValid())
	{
		World = ActorInfo->AvatarActor->GetWorld();
	}
	if (World)
	{
		World->GetTimerManager().ClearTimer(DebugSpeedTimerHandle);
	}

	// 1) Task 정리
	if (WaitDelayTask)
	{
		WaitDelayTask->EndTask();
		WaitDelayTask = nullptr;
	}

	// 2) 로컬 FOV 원복
	RestoreLocalFOV(ActorInfo);

	// 3) 이동속도 원복
	// 3-1) GE 적용했으면 제거
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid() && AppliedGEHandle.IsValid())
	{
		UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
		if (ASC)
		{
			ASC->RemoveActiveGameplayEffect(AppliedGEHandle);
		}
		AppliedGEHandle.Invalidate();

		UE_LOG(LogTemp, Warning, TEXT("[PhaseShift] Removed MoveSpeed GE."));
	}

	// 3-2) 폴백으로 건드린 MaxWalkSpeed 원복
	RestoreCharacterMovementFallback(ActorInfo);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UCA_PhaseShift_Primary::TryApplyMoveSpeedGE(const FGameplayAbilityActorInfo* ActorInfo)
{
	if (!ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
		return false;

	if (!MoveSpeedGameplayEffectClass)
		return false;

	UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
	if (!ASC)
		return false;

	// ✅ Apply 전 값
	const float Before = ASC->GetNumericAttribute(UAuraAttributeSet::GetMoveSpeedAttribute());
	UE_LOG(LogTemp, Warning, TEXT("[PhaseShift] MoveSpeed BEFORE=%.1f"), Before);

	// GE를 Self에 적용
	FGameplayEffectContextHandle Ctx = ASC->MakeEffectContext();
	Ctx.AddSourceObject(this);

	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(MoveSpeedGameplayEffectClass, 1.0f, Ctx);
	if (!SpecHandle.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[PhaseShift] MakeOutgoingSpec FAILED"));
		return false;
	}

	AppliedGEHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

	// ✅ Apply 직후 값
	const float After = ASC->GetNumericAttribute(UAuraAttributeSet::GetMoveSpeedAttribute());
	UE_LOG(LogTemp, Warning, TEXT("[PhaseShift] MoveSpeed AFTER =%.1f | GEHandleValid=%d"),
		After,
		AppliedGEHandle.IsValid() ? 1 : 0
	);

	return AppliedGEHandle.IsValid();
}


bool UCA_PhaseShift_Primary::TryApplyCharacterMovementFallback(const FGameplayAbilityActorInfo* ActorInfo)
{
	if (!ActorInfo)
		return false;

	ACharacter* Ch = Cast<ACharacter>(ActorInfo->AvatarActor.Get());

	if (!Ch)
	{
		if (APlayerController* PC = Cast<APlayerController>(ActorInfo->PlayerController.Get()))
		{
			Ch = Cast<ACharacter>(PC->GetPawn());
		}
	}

	if (!Ch)
		return false;

	UCharacterMovementComponent* MoveComp = Ch->GetCharacterMovement();
	if (!MoveComp)
		return false;

	if (CachedOriginalMaxWalkSpeed < 0.f)
	{
		CachedOriginalMaxWalkSpeed = MoveComp->MaxWalkSpeed;
	}

	MoveComp->MaxWalkSpeed = CachedOriginalMaxWalkSpeed * MoveSpeedMultiplierFallback;
	return true;
}

void UCA_PhaseShift_Primary::RestoreCharacterMovementFallback(const FGameplayAbilityActorInfo* ActorInfo)
{
	if (CachedOriginalMaxWalkSpeed < 0.f)
		return;

	if (!ActorInfo || !ActorInfo->AvatarActor.IsValid())
		return;

	ACharacter* Ch = Cast<ACharacter>(ActorInfo->AvatarActor.Get());
	if (!Ch)
		return;

	UCharacterMovementComponent* MoveComp = Ch->GetCharacterMovement();
	if (!MoveComp)
		return;

	MoveComp->MaxWalkSpeed = CachedOriginalMaxWalkSpeed;
	CachedOriginalMaxWalkSpeed = -1.f;

	UE_LOG(LogTemp, Warning, TEXT("[PhaseShift] Restored MaxWalkSpeed fallback."));
}

void UCA_PhaseShift_Primary::ApplyLocalFOV(const FGameplayAbilityActorInfo* ActorInfo)
{
	if (!ActorInfo || !ActorInfo->IsLocallyControlled())
		return;

	APlayerController* PC = Cast<APlayerController>(ActorInfo->PlayerController.Get());
	if (!PC) return;

	APlayerCameraManager* Cam = PC->PlayerCameraManager;
	if (!Cam) return;

	if (CachedOriginalFOV < 0.f)
	{
		CachedOriginalFOV = Cam->GetFOVAngle();
	}

	Cam->SetFOV(CachedOriginalFOV + FOVDelta);
}

void UCA_PhaseShift_Primary::RestoreLocalFOV(const FGameplayAbilityActorInfo* ActorInfo)
{
	if (CachedOriginalFOV < 0.f)
		return;

	if (!ActorInfo || !ActorInfo->IsLocallyControlled())
	{
		CachedOriginalFOV = -1.f;
		return;
	}

	APlayerController* PC = Cast<APlayerController>(ActorInfo->PlayerController.Get());
	if (!PC)
	{
		CachedOriginalFOV = -1.f;
		return;
	}

	APlayerCameraManager* Cam = PC->PlayerCameraManager;
	if (!Cam)
	{
		CachedOriginalFOV = -1.f;
		return;
	}

	Cam->SetFOV(CachedOriginalFOV);
	CachedOriginalFOV = -1.f;
}