// Fill out your copyright notice in the Description page of Project Settings.


#include "GameBase/AuraPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

AAuraPlayerController::AAuraPlayerController()
{
}

void AAuraPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	//로컬플레이어에 IMC 연결하기
	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsys = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (IMC_Card)
			{
				Subsys->AddMappingContext(IMC_Card,0);
			}
		}
		
	}
}

void AAuraPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	//플레이어 컨트롤러를 EnhancedInputComponent 로 캐스팅시키기
	UEnhancedInputComponent* EIC=Cast<UEnhancedInputComponent>(InputComponent);
	if (!EIC)
	{
		return;
	}
	
	//LBM,RBM,휠 트리거로 처리
	if (IA_CardLMB)
	{
		EIC->BindAction(IA_CardLMB, ETriggerEvent::Triggered,this,&ThisClass::OnCardLMB);
	}
	
	if (IA_CardRMB)
	{
		EIC->BindAction(IA_CardRMB, ETriggerEvent::Triggered,this,&ThisClass::OnCardRMB);
	}
	
	if (IA_CardSelect)
	{
		EIC->BindAction(IA_CardSelect, ETriggerEvent::Triggered, this, &ThisClass::OnCardSelect);
	}
	
	
}

void AAuraPlayerController::OnCardLMB()
{
	
}

void AAuraPlayerController::OnCardRMB()
{
}

void AAuraPlayerController::OnCardSelect(const FInputActionValue& Value)
{
	const float AxisValue = Value.Get<float>();
	const float Sign = FMath::Sign(AxisValue);

	if (Sign > 0.f)
	{
		SelectNext();   // 휠다운 (>)
	}
	else if (Sign < 0.f)
	{
		SelectPrev();   // 휠업 (<)
	}
}



void AAuraPlayerController::SelectNext()
{
	SelectedIndex = (SelectedIndex + 1) % 3;
}

void AAuraPlayerController::SelectPrev()
{
	SelectedIndex = (SelectedIndex + 2) % 3;
}
