#include "Card/AuraCombatCardComponent.h"
#include "Card/DA_AuraCardAbilityMapping.h"
#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/PlayerState.h"
#include "Card/DA_CardDefinition.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "UObject/UObjectGlobals.h"
#include "GameBase/AuraGameInstance.h"



UAuraCombatCardComponent::UAuraCombatCardComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	
	// ===== Ability Mapping Asset (Hard Bind) =====
	static ConstructorHelpers::FObjectFinder<UDA_AuraCardAbilityMapping> MappingFinder(
		TEXT("/Game/_BP/Card/DA_CardAbilityMapping_Default")
	);

	if (MappingFinder.Succeeded())
	{
		AbilityMappingAsset = MappingFinder.Object;
	}
	else
	{
		UE_LOG(LogTemp, Error,
			TEXT("[CombatCard] Failed to load DA_CardAbilityMapping_Default"));
	}
}

bool UAuraCombatCardComponent::UseSlotCard(int32 SlotIndex, ECardUseInput InputType)
{
	// 0) 슬롯 유효성
	if (!Slots.IsValidIndex(SlotIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatCard] UseSlotCard invalid SlotIndex=%d"), SlotIndex);
		return false;
	}

	const FName CardID = Slots[SlotIndex];
	if (CardID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatCard] UseSlotCard slot is empty. SlotIndex=%d"), SlotIndex);
		return false;
	}

	// 1) 매핑 에셋 확인
	if (!AbilityMappingAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatCard] AbilityMappingAsset is null. Set DA_CardAbilityMapping_Default on component."));
		return false;
	}

	// 2) 매핑 조회
	FCardAbilityPair Pair;
	if (!AbilityMappingAsset->GetAbilityPair(CardID, Pair))
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatCard] AbilityPair not found for CardID=%s"), *CardID.ToString());
		return false;
	}

	TSubclassOf<UGameplayAbility> AbilityClass =
		(InputType == ECardUseInput::Primary) ? Pair.PrimaryAbility : Pair.AltAbility;

	if (!AbilityClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatCard] AbilityClass is null. CardID=%s Input=%s"),
			*CardID.ToString(),
			(InputType == ECardUseInput::Primary) ? TEXT("Primary") : TEXT("Alt"));
		return false;
	}

	// 3) ASC 얻기
	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatCard] ASC is null. (Owner must implement IAbilitySystemInterface or provide ASC)"));
		return false;
	}

	// 4) Ability 실행
	const bool bActivated = ASC->TryActivateAbilityByClass(AbilityClass);
	if (!bActivated)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatCard] TryActivateAbilityByClass failed. Ability=%s CardID=%s"),
			*AbilityClass->GetName(),
			*CardID.ToString());
		return false;
	}

	// 5) 실행 성공 후에만 카드 소비/재생산 처리
	//    (TryConsumeSlot 내부에서 Queue/ReproTable 처리까지 이미 하고 있는 구조를 활용)
	FName ConsumedCardID = NAME_None;
	const bool bConsumed = TryConsumeSlot(SlotIndex, ConsumedCardID);

	if (!bConsumed)
	{
		// Ability는 이미 발동된 상태라 여기서 false를 반환하면 UX가 애매해짐.
		// 우선 true 반환 + 경고 로그로 처리.
		UE_LOG(LogTemp, Warning, TEXT("[CombatCard] Ability activated but TryConsumeSlot failed. SlotIndex=%d CardID=%s"),
			SlotIndex, *CardID.ToString());
		return true;
	}

	// 안전 체크: 소비된 카드가 기대한 카드인지
	if (ConsumedCardID != CardID)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatCard] ConsumedCardID mismatch. Expected=%s Actual=%s"),
			*CardID.ToString(), *ConsumedCardID.ToString());
	}

	if (bDebugPrint)
	{
		UE_LOG(LogTemp, Log, TEXT("[CombatCard] UseSlotCard OK. CardID=%s Input=%s"),
			*CardID.ToString(),
			(InputType == ECardUseInput::Primary) ? TEXT("Primary") : TEXT("Alt"));
		DebugDumpState(TEXT("UseSlotCard"));
	}

	return true;
}

void UAuraCombatCardComponent::BeginPlay()
{
	Super::BeginPlay();
	
	if (!AbilityMappingAsset)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[CombatCard] BeginPlay: AbilityMappingAsset is NULL"));
	}
	else
	{
		UE_LOG(LogTemp, Log,
			TEXT("[CombatCard] BeginPlay: AbilityMappingAsset OK (%s)"),
			*AbilityMappingAsset->GetName());
	}
	
	// ===== DEBUG: Slot[0] force Card_GravityField =====
	{
		const FName ForceCardID(TEXT("Card_GravityField"));

		// Slots 배열 크기 보장
		if (Slots.Num() < MaxSlots)
		{
			Slots.SetNum(MaxSlots);
		}

		Slots[0] = ForceCardID;

		// 재생산 테이블에 있으면 제거(= "지금은 사용 가능" 상태로 두기 위한 가장 안전한 방식)
		// IsCardReady() 구현에 따라 '없으면 Ready'로 처리되는 경우가 많음.
		ReproTable.Remove(ForceCardID);

		if (bDebugPrint)
		{
			UE_LOG(LogTemp, Warning, TEXT("[CombatCard][DEBUG] Forced Slot[0] = %s"), *ForceCardID.ToString());
		}
	}
	// ================================================
}

void UAuraCombatCardComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateReproduction(DeltaTime);
	TryAutoFillSlots();
}

UAbilitySystemComponent* UAuraCombatCardComponent::GetOwnerASC() const
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor)
	{
		return nullptr;
	}

	// 1) Owner가 IAbilitySystemInterface를 구현한 경우(가장 정석)
	if (OwnerActor->GetClass()->ImplementsInterface(UAbilitySystemInterface::StaticClass()))
	{
		IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(OwnerActor);
		if (ASI)
		{
			return ASI->GetAbilitySystemComponent();
		}
	}

	// 2) Owner가 PlayerState가 아니고, Pawn/Character쪽에 ASC가 있는 경우도 있으니 한 번 더 시도
	//    (프로젝트에 따라 Owner가 PlayerState가 아닐 수도 있으니까 안전장치)
	if (APawn* PawnOwner = Cast<APawn>(OwnerActor))
	{
		AActor* PawnAsActor = PawnOwner;
		if (PawnAsActor->GetClass()->ImplementsInterface(UAbilitySystemInterface::StaticClass()))
		{
			IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PawnAsActor);
			if (ASI)
			{
				return ASI->GetAbilitySystemComponent();
			}
		}
	}

	// 3) Owner가 PlayerState인데, ASC가 PlayerCharacter에 있을 수도 있어서(네 프로젝트 구조에 따라)
	if (APlayerState* PS = Cast<APlayerState>(OwnerActor))
	{
		if (APawn* Pawn = PS->GetPawn())
		{
			AActor* PawnActor = Pawn;
			if (PawnActor->GetClass()->ImplementsInterface(UAbilitySystemInterface::StaticClass()))
			{
				IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PawnActor);
				if (ASI)
				{
					return ASI->GetAbilitySystemComponent();
				}
			}
		}
	}

	return nullptr;
}

void UAuraCombatCardComponent::InitializeCombatCards(const TArray<FName>& InCombatPoolOrder)
{
	ResetRuntime();
	CombatPoolOrder = InCombatPoolOrder;

	// 카드별 런타임 테이블 초기화(Ready)
	for (const FName& CardID : CombatPoolOrder)
	{
		if (CardID.IsNone())
			continue;

		FCombatCardReproRuntime Runtime;
		Runtime.CardID = CardID;
		Runtime.State = ECombatCardReproState::Ready;
		Runtime.RemainingTime = 0.f;
		Runtime.TotalTime = 0.f;

		ReproTable.Add(CardID, Runtime);
	}

	FillInitialSlots();

	if (bDebugPrint)
	{
		DebugDumpState(TEXT("InitializeCombatCards"));
	}
}

bool UAuraCombatCardComponent::TryConsumeSlot(int32 SlotIndex, FName& OutCardID)
{
	OutCardID = NAME_None;

	if (!Slots.IsValidIndex(SlotIndex))
		return false;

	const FName CardID = Slots[SlotIndex];

	// 빈 슬롯
	if (CardID.IsNone())
		return false;

	// Ready만 사용 가능
	if (!IsCardReady(CardID))
		return false;

	// 1) 슬롯 비우기
	Slots[SlotIndex] = NAME_None;

	// 2) 큐에 적재
	Queue.Add(CardID);

	// 3) 재생산 시작 (CardCost = 재생산 시간(초))
	const float ReproTimeSec = GetReproTimeFromCardCost(CardID);
	StartReproduction(CardID, ReproTimeSec);

	OutCardID = CardID;

	// 4) 즉시 슬롯 자동 채우기 시도
	TryAutoFillSlots();

	if (bDebugPrint)
	{
		DebugDumpState(TEXT("TryConsumeSlot"));
	}

	return true;
}

void UAuraCombatCardComponent::GetSlots(TArray<FName>& OutSlots) const
{
	OutSlots = Slots;
}

void UAuraCombatCardComponent::GetQueue(TArray<FName>& OutQueue) const
{
	OutQueue = Queue;
}

void UAuraCombatCardComponent::GetReproducingCards(TArray<FCombatCardReproRuntime>& OutReproducing) const
{
	OutReproducing.Reset();

	for (const auto& Pair : ReproTable)
	{
		if (Pair.Value.State == ECombatCardReproState::Reproducing)
		{
			OutReproducing.Add(Pair.Value);
		}
	}
}

bool UAuraCombatCardComponent::IsCardReady(FName CardID) const
{
	if (CardID.IsNone())
		return false;

	if (const FCombatCardReproRuntime* Found = ReproTable.Find(CardID))
	{
		return Found->State == ECombatCardReproState::Ready;
	}

	return false;
}

const UDA_CardDefinition* UAuraCombatCardComponent::FindCardDef(FName CardID) const
{
	if (CardID.IsNone())
	{
		return nullptr;
	}
	
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}
	
	UGameInstance* GIBase = World->GetGameInstance();
	if (!GIBase)
	{
		return nullptr;
	}

	UAuraGameInstance* GI = Cast<UAuraGameInstance>(GIBase);
	if (!GI)
	{
		return nullptr;
	}

	const auto* Found = GI->CardRegistry.Find(CardID);
	if (!Found)
	{
		return nullptr;
	}

	// ValueType이 UDA_CardDefinition* 이든 TObjectPtr<UDA_CardDefinition> 이든 처리
	const UDA_CardDefinition* Def = Cast<UDA_CardDefinition>(*Found);
	return Def;
}


float UAuraCombatCardComponent::GetReproTimeFromCardCost(FName CardID) const
{
	// 기본값 폴백
	float Result = DefaultReproTime;
	
	if (const UDA_CardDefinition* Def = FindCardDef(CardID))
	{
		// CardCost를 “재생산 시간(초)”로 해석
		Result = static_cast<float>(FMath::Max(0, Def->CardCost));
	}
	
	return Result;
}


void UAuraCombatCardComponent::ResetRuntime()
{
	CombatPoolOrder.Reset();
	Queue.Reset();
	ReproTable.Reset();

	Slots.SetNum(MaxSlots);
	for (FName& Slot : Slots)
	{
		Slot = NAME_None;
	}
}

void UAuraCombatCardComponent::FillInitialSlots()
{
	// 전략: CombatPoolOrder 앞에서부터 Ready 카드로 슬롯 채우기
	int32 FillIndex = 0;

	for (const FName& CardID : CombatPoolOrder)
	{
		if (FillIndex >= MaxSlots)
			break;

		if (CardID.IsNone())
			continue;

		// ReproTable에 등록된 카드면 기본 Ready
		if (IsCardReady(CardID))
		{
			Slots[FillIndex++] = CardID;
		}
	}
}

void UAuraCombatCardComponent::StartReproduction(FName CardID, float ReproTime)
{
	FCombatCardReproRuntime& Runtime = ReproTable.FindOrAdd(CardID);
	Runtime.CardID = CardID;

	// 0초(또는 음수)는 즉시 Ready 처리
	if (ReproTime <= 0.f)
	{
		Runtime.State = ECombatCardReproState::Ready;
		Runtime.TotalTime = 0.f;
		Runtime.RemainingTime = 0.f;
		return;
	}

	Runtime.State = ECombatCardReproState::Reproducing;
	Runtime.TotalTime = ReproTime;
	Runtime.RemainingTime = ReproTime;
}

void UAuraCombatCardComponent::UpdateReproduction(float DeltaTime)
{
	if (DeltaTime <= 0.f)
		return;

	for (auto& Pair : ReproTable)
	{
		FCombatCardReproRuntime& Runtime = Pair.Value;

		if (Runtime.State != ECombatCardReproState::Reproducing)
			continue;

		Runtime.RemainingTime -= DeltaTime;

		if (Runtime.RemainingTime <= 0.f)
		{
			Runtime.RemainingTime = 0.f;
			Runtime.State = ECombatCardReproState::Ready;
		}
	}
}

void UAuraCombatCardComponent::TryAutoFillSlots()
{
	for (FName& Slot : Slots)
	{
		if (!Slot.IsNone())
			continue;

		FName NextCardID = NAME_None;
		if (PopNextReadyFromQueue(NextCardID))
		{
			Slot = NextCardID;
		}
	}
}

bool UAuraCombatCardComponent::PopNextReadyFromQueue(FName& OutCardID)
{
	OutCardID = NAME_None;

	for (int32 i = 0; i < Queue.Num(); ++i)
	{
		const FName Candidate = Queue[i];

		if (Candidate.IsNone())
			continue;

		if (IsCardReady(Candidate))
		{
			OutCardID = Candidate;
			Queue.RemoveAt(i);
			return true;
		}
	}

	return false;
}

void UAuraCombatCardComponent::DebugDumpState(const TCHAR* Context) const
{
	UE_LOG(LogTemp, Log, TEXT("[AuraCombatCardComponent][%s]"), Context);

	FString SlotsStr;
	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		SlotsStr += FString::Printf(TEXT("S%d=%s "), i, *Slots[i].ToString());
	}
	UE_LOG(LogTemp, Log, TEXT("Slots: %s"), *SlotsStr);

	FString QueueStr;
	for (const FName& Q : Queue)
	{
		QueueStr += Q.ToString() + TEXT(" ");
	}
	UE_LOG(LogTemp, Log, TEXT("Queue: %s"), *QueueStr);
}
