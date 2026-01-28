#include "Card/AuraCombatCardComponent.h"
#include "Card/DA_CardDefinition.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "UObject/UObjectGlobals.h"
#include "GameBase/AuraGameInstance.h"



UAuraCombatCardComponent::UAuraCombatCardComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
}

void UAuraCombatCardComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UAuraCombatCardComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateReproduction(DeltaTime);
	TryAutoFillSlots();
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
