// Fill out your copyright notice in the Description page of Project Settings.

#include "GameBase/AuraPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

#include "GameFramework/PlayerState.h"

#include "GameBase/AuraPlayerState.h"
#include "Card/AuraCombatCardComponent.h"

#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "AbilitySystemInterface.h"


static const FGameplayTag TAG_TargetingActive =
	FGameplayTag::RequestGameplayTag(FName("State.Targeting.Active"));

static UAbilitySystemComponent* GetASC_Preferred(AAuraPlayerController* PC)
{
	if (!PC) return nullptr;

	// 1) PlayerState ASC
	if (AAuraPlayerState* APS = PC->GetPlayerState<AAuraPlayerState>())
	{
		if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(APS))
		{
			if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
			{
				return ASC;
			}
		}
	}

	// 2) Pawn/Character ASC (Ability가 여기 붙는 경우 많음)
	if (APawn* P = PC->GetPawn())
	{
		if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(P))
		{
			if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
			{
				return ASC;
			}
		}
	}

	return nullptr;
}

AAuraPlayerController::AAuraPlayerController()
{
}

void AAuraPlayerController::BeginPlay()
{
	Super::BeginPlay();

	// 마우스 입력 캡처 관련
	bShowMouseCursor = false;

	FInputModeGameOnly Mode;
	Mode.SetConsumeCaptureMouseDown(false);
	SetInputMode(Mode);

	// IMC 연결
	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsys = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (IMC_Card)
			{
				Subsys->AddMappingContext(IMC_Card, 100);
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[AuraPC] BeginPlay OK"));
}

void AAuraPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EIC)
	{
		UE_LOG(LogTemp, Error, TEXT("[CardInput] InputComponent is NOT EnhancedInputComponent. Class=%s"),
			*GetNameSafe(InputComponent));
		return;
	}

	if (IA_CardLMB)
	{
		EIC->BindAction(IA_CardLMB, ETriggerEvent::Started, this, &ThisClass::OnCardLMB);
	}

	if (IA_CardRMB)
	{
		EIC->BindAction(IA_CardRMB, ETriggerEvent::Started, this, &ThisClass::OnCardRMB);
	}

	if (IA_CardSelect)
	{
		EIC->BindAction(IA_CardSelect, ETriggerEvent::Triggered, this, &ThisClass::OnCardSelect);
	}
}

void AAuraPlayerController::OnCardLMB()
{
	UAbilitySystemComponent* ASC = GetASC_Preferred(this);

	UE_LOG(LogTemp, Warning,
		TEXT("[CardInput] LMB PreCheck | ASC=%s ASCPtr=%p Owner=%s HasActive=%d"),
		*GetNameSafe(ASC),
		ASC,
		ASC ? *GetNameSafe(ASC->GetOwner()) : TEXT("None"),
		(ASC && ASC->HasMatchingGameplayTag(TAG_TargetingActive)) ? 1 : 0);

	if (ASC && ASC->HasMatchingGameplayTag(TAG_TargetingActive))
	{
		UE_LOG(LogTemp, Warning, TEXT("[CardInput] LMB -> CONFIRM (Targeting.Active)"));
		ASC->LocalInputConfirm();
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[CardInput] LMB | SelectedIndex=%d"), SelectedIndex);
	TryUseSelectedSlotCard(false);
}

void AAuraPlayerController::OnCardRMB()
{
	UAbilitySystemComponent* ASC = GetASC_Preferred(this);

	UE_LOG(LogTemp, Warning,
		TEXT("[CardInput] RMB PreCheck | ASC=%s ASCPtr=%p Owner=%s HasActive=%d"),
		*GetNameSafe(ASC),
		ASC,
		ASC ? *GetNameSafe(ASC->GetOwner()) : TEXT("None"),
		(ASC && ASC->HasMatchingGameplayTag(TAG_TargetingActive)) ? 1 : 0);

	// ★ 타겟팅 중이면 Cancel로 소비
	if (ASC && ASC->HasMatchingGameplayTag(TAG_TargetingActive))
	{
		UE_LOG(LogTemp, Warning, TEXT("[CardInput] RMB -> CANCEL (Targeting.Active)"));
		ASC->LocalInputCancel();
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[CardInput] RMB | SelectedIndex=%d"), SelectedIndex);
	TryUseSelectedSlotCard(true);
}

void AAuraPlayerController::OnCardSelect(const FInputActionValue& Value)
{
	const float AxisValue = Value.Get<float>();
	const float Sign = FMath::Sign(AxisValue);

	UE_LOG(LogTemp, Warning, TEXT("[CardInput] Wheel Axis=%.3f Sign=%.0f | Before=%d"),
		AxisValue, Sign, SelectedIndex);

	if (Sign > 0.f)
	{
		SelectNext();
	}
	else if (Sign < 0.f)
	{
		SelectPrev();
	}

	UE_LOG(LogTemp, Warning, TEXT("[CardInput] Wheel Axis=%.3f | SelectedIndex=%d"), AxisValue, SelectedIndex);
}

void AAuraPlayerController::SelectNext()
{
	SelectedIndex = (SelectedIndex + 1) % 3;
}

void AAuraPlayerController::SelectPrev()
{
	SelectedIndex = (SelectedIndex + 2) % 3;
}

void AAuraPlayerController::TryUseSelectedSlotCard(bool bAltClick)
{
	AAuraPlayerState* APS = GetPlayerState<AAuraPlayerState>();
	if (!APS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatCard] PlayerState is not AAuraPlayerState"));
		return;
	}

	UAuraCombatCardComponent* CardComp = APS->GetCombatCardComponent();
	if (!CardComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatCard] CombatCardComponent is null"));
		return;
	}

	const int32 SlotIdx = FMath::Clamp(SelectedIndex, 0, 2);
	const ECardUseInput InputType = bAltClick ? ECardUseInput::Alt : ECardUseInput::Primary;

	UE_LOG(LogTemp, Warning, TEXT("[CombatCard] UseSlotCard Request | Slot=%d Click=%s"),
		SlotIdx,
		bAltClick ? TEXT("RMB(Alt)") : TEXT("LMB(Primary)"));

	const bool bOK = CardComp->UseSlotCard(SlotIdx, InputType);

	UE_LOG(LogTemp, Warning, TEXT("[CombatCard] UseSlotCard Result | Slot=%d -> %s"),
		SlotIdx,
		bOK ? TEXT("TRUE") : TEXT("FALSE"));
}
