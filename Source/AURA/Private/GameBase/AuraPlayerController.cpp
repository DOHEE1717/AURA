// Fill out your copyright notice in the Description page of Project Settings.


#include "GameBase/AuraPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/Engine.h"
#include "InputActionValue.h"
#include "GameFramework/PlayerController.h"

AAuraPlayerController::AAuraPlayerController()
{
}

void AAuraPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	//우클릭 할당 해제
	bShowMouseCursor = false;
	
	// 핵심: "마우스 캡처를 위한 첫 MouseDown을 소비하지 않음"
	FInputModeGameOnly Mode;
	Mode.SetConsumeCaptureMouseDown(false); // ✅ RMB/LMB가 캡처로 씹히는 문제 방지
	SetInputMode(Mode);
	
	//로컬플레이어에 IMC 연결하기
	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsys = LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			UE_LOG(LogTemp, Warning, TEXT("[CardInput] AddMappingContext called"));

			if (IMC_Card)
			{
				UE_LOG(LogTemp, Warning, TEXT("[CardInput] AddMappingContext called"));
				Subsys->AddMappingContext(IMC_Card,100);
			}
			
			

		}
		
	}
	
	UE_LOG(LogTemp, Warning, TEXT("플레이어 컨트롤러 연결"));
}

void AAuraPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	UE_LOG(LogTemp, Warning, TEXT("[CardInput] SetupInputComponent called"));
	
	//플레이어 컨트롤러를 EnhancedInputComponent 로 캐스팅시키기
	UEnhancedInputComponent* EIC=Cast<UEnhancedInputComponent>(InputComponent);
	if (!EIC)
	{
		UE_LOG(LogTemp, Error, TEXT("[CardInput] InputComponent is NOT EnhancedInputComponent. Class=%s"),
	   *GetNameSafe(InputComponent));
		return;
	}
	
	UE_LOG(LogTemp, Warning, TEXT("[CardInput] Assets: LMB=%s RMB=%s Select=%s"),
	*GetNameSafe(IA_CardLMB),
	*GetNameSafe(IA_CardRMB),
	*GetNameSafe(IA_CardSelect));

	
	//LBM,RBM,휠 트리거로 처리
	if (IA_CardLMB)
	{
		EIC->BindAction(IA_CardLMB, ETriggerEvent::Started,this,&ThisClass::OnCardLMB);
	}
	
	if (IA_CardRMB)
	{
		EIC->BindAction(IA_CardRMB, ETriggerEvent::Started,this,&ThisClass::OnCardRMB);
	}
	
	if (IA_CardSelect)
	{
		EIC->BindAction(IA_CardSelect, ETriggerEvent::Triggered, this, &ThisClass::OnCardSelect);
	}
	
	
}

void AAuraPlayerController::OnCardLMB()
{
	UE_LOG(LogTemp, Warning, TEXT("[CardInput] LMB Pressed | SelectedIndex=%d"), SelectedIndex);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.5f,
			FColor::Yellow,
			FString::Printf(TEXT("[CardInput] LMB | Sel=%d"), SelectedIndex));
	}
}

void AAuraPlayerController::OnCardRMB()
{
	UE_LOG(LogTemp, Warning, TEXT("[CardInput] RMB Pressed | SelectedIndex=%d"), SelectedIndex);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.5f,
			FColor::Cyan,
			FString::Printf(TEXT("[CardInput] RMB | Sel=%d"), SelectedIndex));
	}
}

void AAuraPlayerController::OnCardSelect(const FInputActionValue& Value)
{
	const float AxisValue = Value.Get<float>();
	const float Sign = FMath::Sign(AxisValue);
	
	UE_LOG(LogTemp, Warning, TEXT("[CardInput] Wheel Axis=%.3f Sign=%.0f | Before=%d"),
	AxisValue, Sign, SelectedIndex);

	if (Sign > 0.f)
	{
		SelectNext();   // 휠다운 (>)
	}
	else if (Sign < 0.f)
	{
		SelectPrev();   // 휠업 (<)
	}
	UE_LOG(LogTemp, Warning, TEXT("[CardInput] Wheel | After=%d"), SelectedIndex);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 1.5f,
			FColor::Green,
			FString::Printf(TEXT("[CardInput] Wheel %.3f | Sel=%d"), AxisValue, SelectedIndex));
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
