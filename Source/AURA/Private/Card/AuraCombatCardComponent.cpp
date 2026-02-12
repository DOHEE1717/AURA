#include "Card/AuraCombatCardComponent.h"
#include "Card/DA_AuraCardAbilityMapping.h"
#include "Card/DA_CardDefinition.h"

#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"

#include "GameFramework/PlayerState.h"
#include "GameBase/AuraGameInstance.h"
#include "UObject/ConstructorHelpers.h"
#include "Engine/World.h"

UAuraCombatCardComponent::UAuraCombatCardComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	static ConstructorHelpers::FObjectFinder<UDA_AuraCardAbilityMapping> MappingFinder(
		TEXT("/Game/_BP/Card/DA_CardAbilityMapping_Default")
	);

	if (MappingFinder.Succeeded())
	{
		AbilityMappingAsset = MappingFinder.Object;
	}
}

void UAuraCombatCardComponent::BeginPlay()
{
	Super::BeginPlay();
	BroadcastUI_All();

	// 디버그
	FString SlotsStr;
	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		SlotsStr += FString::Printf(TEXT("S%d=%s "), i, *Slots[i].ToString());
	}
	UE_LOG(LogTemp, Warning, TEXT("[CombatCard] BeginPlay Slots: %s"), *SlotsStr);

	FString QueueStr;
	for (const FName& Q : Queue)
	{
		QueueStr += Q.ToString() + TEXT(" ");
	}
	UE_LOG(LogTemp, Warning, TEXT("[CombatCard] BeginPlay Queue: %s"), *QueueStr);
}

void UAuraCombatCardComponent::TickComponent(float DeltaTime, ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UpdateReproduction(DeltaTime);
	TryAutoFillSlots();

	UIReproBroadcastAccum += DeltaTime;
	if (UIReproBroadcastAccum >= UIReproBroadcastInterval)
	{
		UIReproBroadcastAccum = 0.f;
		BroadcastUI_Repros();
	}
}

////////////////////////////////////////////////////////////
// ASC
////////////////////////////////////////////////////////////

UAbilitySystemComponent* UAuraCombatCardComponent::GetOwnerASC() const
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return nullptr;

	// IAbilitySystemInterface는 Execute_가 아니라, 캐스트 후 직접 호출
	if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(OwnerActor))
	{
		return ASI->GetAbilitySystemComponent();
	}

	return nullptr;
}

////////////////////////////////////////////////////////////
// Ability Mapping
////////////////////////////////////////////////////////////

TSubclassOf<UGameplayAbility> UAuraCombatCardComponent::GetAbilityClassForCard(FName CardID, ECardUseInput InputType) const
{
	if (!AbilityMappingAsset || CardID.IsNone())
		return nullptr;

	FCardAbilityPair Pair;
	if (!AbilityMappingAsset->GetAbilityPair(CardID, Pair))
		return nullptr;

	return (InputType == ECardUseInput::Primary) ? Pair.PrimaryAbility : Pair.AltAbility;
}

////////////////////////////////////////////////////////////
// Grant (B안)
////////////////////////////////////////////////////////////

void UAuraCombatCardComponent::GrantCombatCardAbilities()
{
	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatCard] GrantCombatCardAbilities: ASC null"));
		return;
	}

	// 서버에서만 GiveAbility (멀티 대비)
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		UE_LOG(LogTemp, Verbose, TEXT("[CombatCard] GrantCombatCardAbilities: skip (no authority)"));
		return;
	}

	if (!AbilityMappingAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatCard] GrantCombatCardAbilities: AbilityMappingAsset null"));
		return;
	}

	// 현재 시스템에 존재 가능한 카드들을 모아 "필요한 Ability"를 서버에서 Give
	TSet<FName> UniqueCards;
	for (const FName& C : CombatPoolOrder) if (!C.IsNone()) UniqueCards.Add(C);
	for (const FName& C : Queue)          if (!C.IsNone()) UniqueCards.Add(C);
	for (const FName& C : Slots)          if (!C.IsNone()) UniqueCards.Add(C);

	int32 GrantedCount = 0;

	for (const FName& CardID : UniqueCards)
	{
		if (TSubclassOf<UGameplayAbility> Pri = GetAbilityClassForCard(CardID, ECardUseInput::Primary))
		{
			if (!GrantedAbilityClasses.Contains(Pri))
			{
				ASC->GiveAbility(FGameplayAbilitySpec(Pri, 1, INDEX_NONE, this));
				GrantedAbilityClasses.Add(Pri);
				GrantedCount++;
				UE_LOG(LogTemp, Warning, TEXT("[CombatCard] Granted PRI: %s (%s)"), *CardID.ToString(), *GetNameSafe(Pri));
			}
		}

		if (TSubclassOf<UGameplayAbility> Alt = GetAbilityClassForCard(CardID, ECardUseInput::Alt))
		{
			if (!GrantedAbilityClasses.Contains(Alt))
			{
				ASC->GiveAbility(FGameplayAbilitySpec(Alt, 1, INDEX_NONE, this));
				GrantedAbilityClasses.Add(Alt);
				GrantedCount++;
				UE_LOG(LogTemp, Warning, TEXT("[CombatCard] Granted ALT: %s (%s)"), *CardID.ToString(), *GetNameSafe(Alt));
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[CombatCard] GrantCombatCardAbilities done. Count=%d"), GrantedCount);
}

////////////////////////////////////////////////////////////
// Use Slot (Activate 포함)
////////////////////////////////////////////////////////////

bool UAuraCombatCardComponent::UseSlotCard(int32 SlotIndex, ECardUseInput InputType)
{
	if (!Slots.IsValidIndex(SlotIndex)) return false;

	const FName CardID = Slots[SlotIndex];
	if (CardID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatCard] UseSlotCard FAILED: EMPTY Slot=%d"), SlotIndex);
		return false;
	}

	if (!IsCardReady(CardID))
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatCard] UseSlotCard FAILED: NOT READY Slot=%d Card=%s"), SlotIndex, *CardID.ToString());
		return false;
	}

	// 1) 먼저 발동 시도
	const bool bActivated = TryActivateCardAbility(CardID, InputType);
	if (!bActivated)
	{
		// 발동 실패면 절대 소모하지 않음
		UE_LOG(LogTemp, Warning, TEXT("[CombatCard] UseSlotCard FAILED: Activate 실패 -> No Consume (Slot=%d Card=%s)"),
			SlotIndex, *CardID.ToString());

		BroadcastUI_All();
		return false;
	}

	// 2) 발동 성공한 경우에만 소모 + Repro/Queue
	FName Dummy;
	const bool bConsumed = TryConsumeSlot(SlotIndex, Dummy);

	BroadcastUI_All();

	UE_LOG(LogTemp, Warning, TEXT("[CombatCard] UseSlotCard OK: Slot=%d Card=%s Consumed=%d"),
		SlotIndex, *CardID.ToString(), bConsumed ? 1 : 0);

	return bConsumed;
}

////////////////////////////////////////////////////////////
// 기존 카드 로직 
////////////////////////////////////////////////////////////

bool UAuraCombatCardComponent::TryConsumeSlot(int32 SlotIndex, FName& OutCardID)
{
	OutCardID = NAME_None;

	if (!Slots.IsValidIndex(SlotIndex)) return false;
	const FName CardID = Slots[SlotIndex];
	if (CardID.IsNone()) return false;

	Slots[SlotIndex] = NAME_None;
	
	

	UE_LOG(LogTemp, Warning, TEXT("[CombatCard] Consumed Slot=%d Card=%s -> Queue+Repro"),
		SlotIndex, *CardID.ToString());

	Queue.Add(CardID);

	StartReproduction(CardID, GetReproTimeFromCardCost(CardID));
	TryAutoFillSlots();

	OutCardID = CardID;
	return true;
}

void UAuraCombatCardComponent::InitializeCombatCards(const TArray<FName>& InCombatPoolOrder)
{
	ResetRuntime();
	CombatPoolOrder = InCombatPoolOrder;

	for (const FName& CardID : CombatPoolOrder)
	{
		FCombatCardReproRuntime R;
		R.CardID = CardID;
		R.State = ECombatCardReproState::Ready;
		ReproTable.Add(CardID, R);
	}

	FillInitialSlots();
	
	// (추가) 초기 Queue 시드: 슬롯에 못 들어간 나머지 카드들은 Queue로 보관
	Queue.Reset();

	for (int32 i = 0; i < CombatPoolOrder.Num(); ++i)
	{
		const FName CardID = CombatPoolOrder[i];
		if (CardID.IsNone()) continue;

		// 슬롯에 이미 들어간 애는 스킵
		if (Slots.Contains(CardID))
		{
			continue;
		}

		Queue.Add(CardID);
	}

	UE_LOG(LogTemp, Warning, TEXT("[CombatCard] Seed Queue from Pool. QueueNum=%d"), Queue.Num());

	GrantCombatCardAbilities();

	BroadcastUI_All();
	
	UE_LOG(LogTemp, Warning, TEXT("[CombatCard] InitializeCombatCards done. Authority=%d"),
		(GetOwner() && GetOwner()->HasAuthority()) ? 1 : 0);
}

bool UAuraCombatCardComponent::IsCardReady(FName CardID) const
{
	if (const FCombatCardReproRuntime* R = ReproTable.Find(CardID))
	{
		return R->State == ECombatCardReproState::Ready;
	}
	return false;
}

// ================= UI SNAPSHOT =================

TArray<FCombatSlotUIData> UAuraCombatCardComponent::BuildSlotsUISnapshot() const
{
	TArray<FCombatSlotUIData> Out;
	for (int32 i = 0; i < MaxSlots; ++i)
	{
		FCombatSlotUIData D;
		const FName CardID = Slots.IsValidIndex(i) ? Slots[i] : NAME_None;
		D.CardID = CardID;

		if (CardID.IsNone())
		{
			D.StateText = TEXT("EMPTY");
		}
		else if (const FCombatCardReproRuntime* R = ReproTable.Find(CardID))
		{
			D.StateText = (R->State == ECombatCardReproState::Ready) ? TEXT("READY") : TEXT("REPRO");
			D.Remaining = R->RemainingTime;
			D.Duration = R->TotalTime;
		}

		Out.Add(D);
	}
	return Out;
}

TArray<FCombatReproUIData> UAuraCombatCardComponent::BuildReprosUISnapshot() const
{
	TArray<FCombatReproUIData> Out;

	for (const auto& Pair : ReproTable)
	{
		const FCombatCardReproRuntime& R = Pair.Value;
		if (R.State != ECombatCardReproState::Reproducing) continue;

		FCombatReproUIData D;
		D.CardID = R.CardID;
		D.Remaining = R.RemainingTime;
		D.Duration = R.TotalTime;
		Out.Add(D);
	}

	Out.Sort([](const FCombatReproUIData& A, const FCombatReproUIData& B)
	{
		return A.Remaining > B.Remaining;
	});

	return Out;
}

FName UAuraCombatCardComponent::BuildNextUISnapshot() const
{
	for (const FName& C : Queue)
	{
		if (IsCardReady(C))
			return C;
	}
	return Queue.Num() > 0 ? Queue[0] : NAME_None;
}

// ================= UI BROADCAST =================

void UAuraCombatCardComponent::BroadcastUI_Slots()
{
	OnCombatSlotsUIChanged.Broadcast(BuildSlotsUISnapshot());
}

void UAuraCombatCardComponent::BroadcastUI_Repros()
{
	OnCombatReproUIChanged.Broadcast(BuildReprosUISnapshot());
}

void UAuraCombatCardComponent::BroadcastUI_Next()
{
	OnCombatNextUIChanged.Broadcast(BuildNextUISnapshot());
}

void UAuraCombatCardComponent::BroadcastUI_All()
{
	BroadcastUI_Slots();
	BroadcastUI_Repros();
	BroadcastUI_Next();
}

// ================= INTERNAL =================

void UAuraCombatCardComponent::ResetRuntime()
{
	CombatPoolOrder.Reset();
	Queue.Reset();
	ReproTable.Reset();
	Slots.SetNum(MaxSlots);

	// (Grant 캐시도 초기화)
	GrantedAbilityClasses.Reset();
}

void UAuraCombatCardComponent::FillInitialSlots()
{
	int32 Idx = 0;
	for (const FName& CardID : CombatPoolOrder)
	{
		if (Idx >= MaxSlots) break;
		if (IsCardReady(CardID))
		{
			Slots[Idx++] = CardID;
		}
	}
}

void UAuraCombatCardComponent::StartReproduction(FName CardID, float ReproTime)
{
	FCombatCardReproRuntime& R = ReproTable.FindOrAdd(CardID);
	R.CardID = CardID;
	R.TotalTime = ReproTime;
	R.RemainingTime = ReproTime;
	R.State = (ReproTime <= 0.f) ? ECombatCardReproState::Ready : ECombatCardReproState::Reproducing;

	UE_LOG(LogTemp, Warning, TEXT("[CombatCard] StartRepro Card=%s Time=%.2f State=%s"),
		*CardID.ToString(),
		ReproTime,
		(ReproTime <= 0.f) ? TEXT("Ready") : TEXT("Reproducing"));
}

void UAuraCombatCardComponent::UpdateReproduction(float DeltaTime)
{
	for (auto& Pair : ReproTable)
	{
		FCombatCardReproRuntime& R = Pair.Value;
		if (R.State != ECombatCardReproState::Reproducing) continue;

		R.RemainingTime -= DeltaTime;
		if (R.RemainingTime <= 0.f)
		{
			R.RemainingTime = 0.f;
			R.State = ECombatCardReproState::Ready;
		}
	}
}

void UAuraCombatCardComponent::TryAutoFillSlots()
{
	bool bSlotChanged = false;

	for (FName& Slot : Slots)
	{
		if (!Slot.IsNone()) continue;

		FName Next;
		if (PopNextReadyFromQueue(Next))
		{
			Slot = Next;
			bSlotChanged = true;
		}
	}

	// 슬롯이 바뀌었으면 즉시 UI 반영 (EMPTY 잔상 방지)
	if (bSlotChanged)
	{
		BroadcastUI_Slots();
		BroadcastUI_Next();
	}
}

bool UAuraCombatCardComponent::PopNextReadyFromQueue(FName& OutCardID)
{
	for (int32 i = 0; i < Queue.Num(); ++i)
	{
		if (IsCardReady(Queue[i]))
		{
			OutCardID = Queue[i];
			Queue.RemoveAt(i);
			return true;
		}
	}
	return false;
}

const UDA_CardDefinition* UAuraCombatCardComponent::FindCardDef(FName CardID) const
{
	if (UAuraGameInstance* GI = GetWorld()->GetGameInstance<UAuraGameInstance>())
	{
		if (const auto* Found = GI->CardRegistry.Find(CardID))
		{
			return Cast<UDA_CardDefinition>(*Found);
		}
	}
	return nullptr;
}

float UAuraCombatCardComponent::GetReproTimeFromCardCost(FName CardID) const
{
	if (const UDA_CardDefinition* Def = FindCardDef(CardID))
	{
		const float Time = FMath::Max(0.f, (float)Def->CardCost);

		UE_LOG(LogTemp, Warning, TEXT("[CombatCard] ReproTimeFromCost | Card=%s Cost=%d Time=%.2f"),
			*CardID.ToString(), Def->CardCost, Time);

		return Time;
	}

	UE_LOG(LogTemp, Warning, TEXT("[CombatCard] ReproTimeFromCost | Card=%s Def NOT FOUND -> Default=%.2f"),
		*CardID.ToString(), DefaultReproTime);

	return DefaultReproTime;
}

bool UAuraCombatCardComponent::EnsureAbilityGranted(UAbilitySystemComponent* ASC, TSubclassOf<UGameplayAbility> AbilityClass)
{
	if (!ASC || !AbilityClass) return false;

	// 이미 Spec이 있으면 OK
	if (ASC->FindAbilitySpecFromClass(AbilityClass))
	{
		return true;
	}

	// 서버가 아니면 Give를 못 하므로, 여기서는 실패 처리
	// (서버에서 GrantCombatCardAbilities가 먼저 돌아야 함)
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatCard] EnsureAbilityGranted FAILED (no authority): %s"), *GetNameSafe(AbilityClass));
		return false;
	}

	ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE, this));
	GrantedAbilityClasses.Add(AbilityClass);

	UE_LOG(LogTemp, Warning, TEXT("[CombatCard] EnsureAbilityGranted GIVE: %s"), *GetNameSafe(AbilityClass));
	return ASC->FindAbilitySpecFromClass(AbilityClass) != nullptr;
}

bool UAuraCombatCardComponent::TryActivateCardAbility(FName CardID, ECardUseInput InputType)
{
	UAbilitySystemComponent* ASC = GetOwnerASC();
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatCard] Activate FAILED: ASC null"));
		return false;
	}

	const TSubclassOf<UGameplayAbility> AbilityClass = GetAbilityClassForCard(CardID, InputType);
	if (!AbilityClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatCard] Activate FAILED: AbilityClass null Card=%s"), *CardID.ToString());
		return false;
	}

	// Grant가 안 돼 있으면 서버에서만 보강 가능
	if (!EnsureAbilityGranted(ASC, AbilityClass))
	{
		UE_LOG(LogTemp, Warning, TEXT("[CombatCard] Activate FAILED: Ability not granted yet (%s)"), *GetNameSafe(AbilityClass));
		return false;
	}

	const bool bActivated = ASC->TryActivateAbilityByClass(AbilityClass);

	UE_LOG(LogTemp, Warning, TEXT("[CombatCard] TryActivate Card=%s Input=%s Ability=%s Result=%d"),
		*CardID.ToString(),
		(InputType == ECardUseInput::Primary) ? TEXT("PRI") : TEXT("ALT"),
		*GetNameSafe(AbilityClass),
		bActivated ? 1 : 0);

	return bActivated;
}