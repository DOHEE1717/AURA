// Fill out your copyright notice in the Description page of Project Settings.

#include "GameBase/AuraPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "Abilities/GameplayAbility.h"

#include "GameBase/AuraPlayerState.h"
#include "Card/AuraCombatCardComponent.h"

static UAbilitySystemComponent* GetASC_Safe(APlayerController* PC)
{
	if (!PC) return nullptr;

	// PlayerState 우선 (ASC in PlayerState)
	if (APlayerState* PS = PC->GetPlayerState<APlayerState>())
	{
		if (UAbilitySystemComponent* ASC = PS->FindComponentByClass<UAbilitySystemComponent>())
		{
			return ASC;
		}

		if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PS))
		{
			return ASI->GetAbilitySystemComponent();
		}
	}

	// Pawn 대비
	if (APawn* Pawn = PC->GetPawn())
	{
		if (UAbilitySystemComponent* ASC = Pawn->FindComponentByClass<UAbilitySystemComponent>())
		{
			return ASC;
		}

		if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(Pawn))
		{
			return ASI->GetAbilitySystemComponent();
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
	UE_LOG(LogTemp, Warning, TEXT("[CardInput] LMB | SelectedIndex=%d"), SelectedIndex);
	TryUseSelectedSlotCard(false);
}

void AAuraPlayerController::OnCardRMB()
{
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

	// 슬롯 사용 시도
	FName OutCardID = NAME_None;
	const bool bConsumed = CardComp->TryConsumeSlot(SelectedIndex, OutCardID);

	if (!bConsumed || OutCardID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatCard] TryConsumeSlot FAILED | Slot=%d"), SelectedIndex);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[CombatCard] Consumed | Slot=%d CardID=%s Click=%s"),
		SelectedIndex,
		*OutCardID.ToString(),
		bAltClick ? TEXT("RMB(Alt)") : TEXT("LMB(Primary)"));

	// CardID -> AbilityClass 매핑
	const TMap<FName, TSubclassOf<UGameplayAbility>>& MapToUse = bAltClick ? AltAbilityMap : PrimaryAbilityMap;

	const TSubclassOf<UGameplayAbility>* FoundAbilityClass = MapToUse.Find(OutCardID);
	if (!FoundAbilityClass || !(*FoundAbilityClass))
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatCard] No Ability mapped for CardID=%s (%s)"),
			*OutCardID.ToString(),
			bAltClick ? TEXT("Alt") : TEXT("Primary"));
		return;
	}

	// ASC로 Ability 실행
	UAbilitySystemComponent* ASC = GetASC_Safe(this);
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatCard] ASC is null"));
		return;
	}

	const bool bActivated = ASC->TryActivateAbilityByClass((*FoundAbilityClass));
	UE_LOG(LogTemp, Warning, TEXT("[CombatCard] ActivateAbility CardID=%s Ability=%s -> %s"),
		*OutCardID.ToString(),
		*GetNameSafe((*FoundAbilityClass).Get()),
		bActivated ? TEXT("TRUE") : TEXT("FALSE"));
}
