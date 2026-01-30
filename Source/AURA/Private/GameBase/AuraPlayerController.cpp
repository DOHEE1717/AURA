// Fill out your copyright notice in the Description page of Project Settings.

#include "GameBase/AuraPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

#include "GameFramework/PlayerState.h"

#include "GameBase/AuraPlayerState.h"
#include "Card/AuraCombatCardComponent.h"



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
