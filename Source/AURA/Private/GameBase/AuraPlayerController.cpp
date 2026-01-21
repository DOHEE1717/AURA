// Fill out your copyright notice in the Description page of Project Settings.


#include "GameBase/AuraPlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/Engine.h"
#include "InputActionValue.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayAbilitySpec.h"
#include "GameBase/AuraPlayerState.h"
#include "GameFramework/PlayerController.h"


static UAbilitySystemComponent* GetASC_Safe(APlayerController* PC)
{
	if (!PC) return nullptr;

	// PlayerState 우선 (너희 구조: ASC in PlayerState)
	if (APlayerState* PS = PC->GetPlayerState<APlayerState>())
	{
		// (A) 컴포넌트로 붙어있는 경우
		if (UAbilitySystemComponent* ASC = PS->FindComponentByClass<UAbilitySystemComponent>())
		{
			return ASC;
		}

		// (B) AbilitySystemInterface 구현한 경우
		if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PS))
		{
			return ASI->GetAbilitySystemComponent();
		}
	}

	// 2) Pawn 쪽도 혹시 대비
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
	
	// ===== TestSlot 초기화: 3칸 보장 + 인덱스 1에 Fireball 꽂기 =====
	TestSlotAbilities.SetNum(3);
	TestSlotAbilities[1] = FireballAbilityClass;

	UE_LOG(LogTemp, Warning, TEXT("[CardTest] SlotInit: [0]=%s [1]=%s [2]=%s"),
		*GetNameSafe(TestSlotAbilities[0].Get()),
		*GetNameSafe(TestSlotAbilities[1].Get()),
		*GetNameSafe(TestSlotAbilities[2].Get()));
	
	// ===== Step3: 테스트용 GiveAbility (Fireball) + ActorInfo 초기화 =====
	if (AAuraPlayerState* APS = GetPlayerState<AAuraPlayerState>())
	{
		if (UAbilitySystemComponent* ASC = APS->GetASC())
		{
			// (중요) Owner=PlayerState, Avatar=Pawn 구조 초기화
			// 이미 다른 곳에서 InitAbilityActorInfo를 확실히 하고 있어도, 이건 테스트 단계에선 안전하게 넣어도 됨
			ASC->InitAbilityActorInfo(APS, GetPawn());

			// 서버 권한에서만 GiveAbility (싱글/리슨서버 포함)
			if (ASC->GetOwnerRole() == ROLE_Authority)
			{
				if (FireballAbilityClass)
				{
					// 중복 지급 방지
					bool bAlreadyHas = false;
					for (const FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
					{
						if (Spec.Ability && Spec.Ability->GetClass() == FireballAbilityClass.Get())
						{
							bAlreadyHas = true;
							break;
						}
					}

					if (!bAlreadyHas)
					{
						FGameplayAbilitySpec Spec(FireballAbilityClass, 1, 0);
						ASC->GiveAbility(Spec);

						UE_LOG(LogTemp, Warning, TEXT("[CardTest] GiveAbility Fireball OK: %s"),
							*GetNameSafe(FireballAbilityClass.Get()));
					}
					else
					{
						UE_LOG(LogTemp, Warning, TEXT("[CardTest] Fireball already granted"));
					}
				}
				else
				{
					UE_LOG(LogTemp, Warning, TEXT("[CardTest] FireballAbilityClass=None (cannot GiveAbility)"));
				}
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[CardTest] Not Authority - skipping GiveAbility"));
			}
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[CardTest] APS->GetASC() returned null"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[CardTest] PlayerState is not AAuraPlayerState (check GameMode PlayerStateClass)"));
	}
	
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

	// ===== Step2: 슬롯 Ability 실행 =====
	if (!TestSlotAbilities.IsValidIndex(SelectedIndex) || !TestSlotAbilities[SelectedIndex])
	{
		UE_LOG(LogTemp, Warning, TEXT("[CardTest] Slot %d is empty"), SelectedIndex);
		return;
	}

	UAbilitySystemComponent* ASC = GetASC_Safe(this);
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CardTest] ASC is null (PlayerState/Pawn 모두 실패)"));
		return;
	}

	TSubclassOf<UGameplayAbility> AbilityClass = TestSlotAbilities[SelectedIndex];
	const bool bActivated = ASC->TryActivateAbilityByClass(AbilityClass);

	UE_LOG(LogTemp, Warning, TEXT("[CardTest] TryActivateAbilityByClass(%s) -> %s"),
		*GetNameSafe(AbilityClass.Get()),
		bActivated ? TEXT("TRUE") : TEXT("FALSE"));
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
