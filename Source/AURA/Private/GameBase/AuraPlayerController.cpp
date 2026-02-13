#include "GameBase/AuraPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

#include "GameFramework/PlayerState.h"
#include "Blueprint/UserWidget.h" // CreateWidget

#include "UI/AuraCombatHUDWidget.h"
#include "GameBase/AuraPlayerState.h"
#include "Card/AuraCombatCardComponent.h"

#include "InputMappingContext.h"
#include "InputAction.h"
#include "UObject/ConstructorHelpers.h"

#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "AbilitySystemInterface.h"

#include "Card/CardAbility/OrbitalStrike/OrbitalReconComponent.h"
#include "Card/CardAbility/OrbitalStrike/OrbitalReconActor.h"

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

	// 2) Pawn/Character ASC
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
	// ===== Card Input Assets (Hard Reference) =====

	// IMC_Card
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> IMC_Card_Finder(
		TEXT("/Game/_BP/Input/IMC_Card")
	);
	if (IMC_Card_Finder.Succeeded())
	{
		IMC_Card = IMC_Card_Finder.Object;
	}

	// IA_CardLMB
	static ConstructorHelpers::FObjectFinder<UInputAction> IA_LMB_Finder(
		TEXT("/Game/_BP/Input/IA_CardLMB")
	);
	if (IA_LMB_Finder.Succeeded())
	{
		IA_CardLMB = IA_LMB_Finder.Object;
	}

	// IA_CardRMB
	static ConstructorHelpers::FObjectFinder<UInputAction> IA_RMB_Finder(
		TEXT("/Game/_BP/Input/IA_CardRMB")
	);
	if (IA_RMB_Finder.Succeeded())
	{
		IA_CardRMB = IA_RMB_Finder.Object;
	}

	// IA_CardSelect (휠)
	static ConstructorHelpers::FObjectFinder<UInputAction> IA_Select_Finder(
		TEXT("/Game/_BP/Input/IA_CardSelect")
	);
	if (IA_Select_Finder.Succeeded())
	{
		IA_CardSelect = IA_Select_Finder.Object;
	}

	// IMC_Recon
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> IMC_Recon_Finder(
		TEXT("/Game/_BP/Input/Recon/IMC_Recon.IMC_Recon")
	);
	if (IMC_Recon_Finder.Succeeded())
	{
		IMC_Recon = IMC_Recon_Finder.Object;
	}

	// IA_Recon_Move
	static ConstructorHelpers::FObjectFinder<UInputAction> IA_ReconMove_Finder(
		TEXT("/Game/_BP/Input/Recon/IA_Recon_Move.IA_Recon_Move")
	);
	if (IA_ReconMove_Finder.Succeeded())
	{
		IA_ReconMove = IA_ReconMove_Finder.Object;
	}

	// IA_Recon_Zoom
	static ConstructorHelpers::FObjectFinder<UInputAction> IA_ReconZoom_Finder(
		TEXT("/Game/_BP/Input/Recon/IA_Recon_Zoom.IA_Recon_Zoom")
	);
	if (IA_ReconZoom_Finder.Succeeded())
	{
		IA_ReconZoom = IA_ReconZoom_Finder.Object;
	}

	// IA_Recon_Exit
	static ConstructorHelpers::FObjectFinder<UInputAction> IA_ReconExit_Finder(
		TEXT("/Game/_BP/Input/Recon/IA_Recon_Exit.IA_Recon_Exit")
	);
	if (IA_ReconExit_Finder.Succeeded())
	{
		IA_ReconExit = IA_ReconExit_Finder.Object;
	}
	
	// ===== Combat HUD (Hard Reference) =====
	static ConstructorHelpers::FClassFinder<UUserWidget> CombatHUDClassFinder(
		TEXT("/Game/_BP/UI/WBP_Combat")
	);

	if (CombatHUDClassFinder.Succeeded())
	{
		CombatHUDClass = CombatHUDClassFinder.Class;
		UE_LOG(LogTemp, Warning, TEXT("[HUD] Hard Loaded CombatHUDClass = %s"), *GetNameSafe(CombatHUDClass));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[HUD] CombatHUDClass is NULL. Hard load failed or asset path is wrong."));
	}
	
}

void AAuraPlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	UE_LOG(LogTemp, Error, TEXT("GI Class = %s"),
	*GetGameInstance()->GetClass()->GetName());
	
	UGameInstance* GI = GetGameInstance();
	UE_LOG(LogTemp, Error, TEXT("GI Class=%s | Super=%s"),
		*GetNameSafe(GI ? GI->GetClass() : nullptr),
		*GetNameSafe(GI ? GI->GetClass()->GetSuperClass() : nullptr));

	// 마우스 입력 캡처 관련
	bShowMouseCursor = false;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;

	FInputModeGameOnly Mode;
	Mode.SetConsumeCaptureMouseDown(false);
	SetInputMode(Mode);

	// IMC 연결
	if (ULocalPlayer* LP = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
			LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (IMC_Card)
			{
				Subsystem->AddMappingContext(IMC_Card, 1);
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[AuraPC] BeginPlay OK"));

	// =====================================================================
	// ✅ (추가) Combat HUD 생성 + CombatCardComponent 바인딩
	// =====================================================================
	if (!IsLocalController())
	{
		return;
	}

	if (!CombatHUDClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HUD] CombatHUDClass is NULL. BP_AuraPlayerController에서 WBP_Combat 지정 필요."));
		return;
	}

	CombatHUDWidget = CreateWidget<UAuraCombatHUDWidget>(this, CombatHUDClass);
	if (!CombatHUDWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("[HUD] CreateWidget failed. CombatHUDClass=%s"), *GetNameSafe(CombatHUDClass));
		return;
	}

	CombatHUDWidget->AddToViewport(0);

	UE_LOG(LogTemp, Warning, TEXT("[HUD] Created Widget Class=%s | Super=%s"),
		*CombatHUDWidget->GetClass()->GetName(),
		*GetNameSafe(CombatHUDWidget->GetClass()->GetSuperClass()));

	AAuraPlayerState* APS = GetPlayerState<AAuraPlayerState>();
	if (!APS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HUD] PlayerState is NULL at BeginPlay."));
		return;
	}

	UAuraCombatCardComponent* CardComp = APS->GetCombatCardComponent();
	if (!CardComp)
	{
		// 혹시 GetCombatCardComponent()가 없거나 반환이 null이면 Find로도 한 번 더 시도
		CardComp = APS->FindComponentByClass<UAuraCombatCardComponent>();
	}

	if (!CardComp)
	{
		UE_LOG(LogTemp, Error, TEXT("[HUD] UAuraCombatCardComponent not found on PlayerState=%s"), *APS->GetName());
		return;
	}

	CombatHUDWidget->BindCombatComponent(CardComp);
	CombatHUDWidget->SetSelectedSlotIndex(SelectedIndex);

	UE_LOG(LogTemp, Warning, TEXT("[HUD] BindCombatComponent OK."));

	// 위젯 트리에 실제로 존재하는지(= BindWidget 이름 불일치 원인 확정용)
	UE_LOG(LogTemp, Warning, TEXT("[HUD] WidgetFromName Slot1=%s Slot2=%s Slot3=%s ReproList=%s"),
		CombatHUDWidget->GetWidgetFromName(TEXT("TXT_Slot1_CardID")) ? TEXT("FOUND") : TEXT("NOT_FOUND"),
		CombatHUDWidget->GetWidgetFromName(TEXT("TXT_Slot2_CardID")) ? TEXT("FOUND") : TEXT("NOT_FOUND"),
		CombatHUDWidget->GetWidgetFromName(TEXT("TXT_Slot3_CardID")) ? TEXT("FOUND") : TEXT("NOT_FOUND"),
		CombatHUDWidget->GetWidgetFromName(TEXT("SB_ReproList")) ? TEXT("FOUND") : TEXT("NOT_FOUND"));
	// =====================================================================
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

	// ===== Recon Bindings =====
	if (IA_ReconMove)
	{
		EIC->BindAction(IA_ReconMove, ETriggerEvent::Triggered, this, &ThisClass::OnReconMove);
	}

	if (IA_ReconZoom)
	{
		EIC->BindAction(IA_ReconZoom, ETriggerEvent::Triggered, this, &ThisClass::OnReconZoom);
	}

	if (IA_ReconExit)
	{
		EIC->BindAction(IA_ReconExit, ETriggerEvent::Started, this, &ThisClass::OnReconExit);
	}
}

void AAuraPlayerController::OnCardLMB()
{
	if (IsReconViewActive())
	{
		return;
	}

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
	if (IsReconViewActive())
	{
		return;
	}

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
	if (IsReconViewActive())
	{
		return;
	}

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
	
	if (CombatHUDWidget)
	{
		CombatHUDWidget->SetSelectedSlotIndex(SelectedIndex);
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

bool AAuraPlayerController::IsReconViewActive() const
{
	if (AAuraPlayerState* APS = GetPlayerState<AAuraPlayerState>())
	{
		if (UOrbitalReconComponent* ReconComp = APS->FindComponentByClass<UOrbitalReconComponent>())
		{
			return ReconComp->GetReconViewActor() != nullptr;
		}
	}
	return false;
}

void AAuraPlayerController::OnReconMove(const FInputActionValue& Value)
{
	if (!IsReconViewActive()) return;

	AAuraPlayerState* APS = GetPlayerState<AAuraPlayerState>();
	if (!APS) return;

	UOrbitalReconComponent* ReconComp = APS->FindComponentByClass<UOrbitalReconComponent>();
	if (!ReconComp) return;

	AOrbitalReconActor* ViewActor = ReconComp->GetReconViewActor();
	if (!ViewActor) return;

	const FVector2D Axis = Value.Get<FVector2D>();
	const float DT = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.f;

	ViewActor->ApplyMoveInput(Axis, DT);
}

void AAuraPlayerController::OnReconZoom(const FInputActionValue& Value)
{
	if (!IsReconViewActive()) return;

	AAuraPlayerState* APS = GetPlayerState<AAuraPlayerState>();
	if (!APS) return;

	UOrbitalReconComponent* ReconComp = APS->FindComponentByClass<UOrbitalReconComponent>();
	if (!ReconComp) return;

	AOrbitalReconActor* ViewActor = ReconComp->GetReconViewActor();
	if (!ViewActor) return;

	const float Axis = Value.Get<float>();
	const float DT = GetWorld() ? GetWorld()->GetDeltaSeconds() : 0.f;

	ViewActor->ApplyZoomInput(Axis, DT);
}

void AAuraPlayerController::OnReconExit()
{
	UE_LOG(LogTemp, Warning, TEXT("[ReconPC] OnReconMove fired"));

	AAuraPlayerState* APS = GetPlayerState<AAuraPlayerState>();
	if (!APS) return;

	if (UOrbitalReconComponent* ReconComp = APS->FindComponentByClass<UOrbitalReconComponent>())
	{
		ReconComp->CloseReconView();
	}
}