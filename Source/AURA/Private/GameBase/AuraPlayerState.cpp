// AuraPlayerState.cpp

#include "GameBase/AuraPlayerState.h"

#include "AbilitySystemComponent.h"
#include "GameBase/AuraAttributeSet.h"

#include "Card/AuraCombatCardComponent.h"
#include "Abilities/GameplayAbility.h"

#include "Card/CardAbility/OrbitalStrike/OrbitalReconComponent.h"
#include "Card/CardAbility/PhaseShift/PhaseShiftRecallComponent.h"

AAuraPlayerState::AAuraPlayerState()
{
	bReplicates = true;

	// ASC
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// AttributeSet
	AttributeSet = CreateDefaultSubobject<UAuraAttributeSet>(TEXT("AuraAttributeSet"));

	// Combat Card Component
	CombatCardComponent = CreateDefaultSubobject<UAuraCombatCardComponent>(TEXT("CombatCardComponent"));

	// Orbital Recon
	OrbitalReconComp = CreateDefaultSubobject<UOrbitalReconComponent>(TEXT("OrbitalReconComp"));

	// PhaseShift
	PhaseShiftRecallComp = CreateDefaultSubobject<UPhaseShiftRecallComponent>(TEXT("PhaseShiftRecallComp"));
}

UAbilitySystemComponent* AAuraPlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AAuraPlayerState::BeginPlay()
{
	Super::BeginPlay();

	// 1) CombatCardComponent 가져오기
	UAuraCombatCardComponent* CardComp = GetCombatCardComponent();
	if (!CardComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AuraPS] BeginPlay: CombatCardComponent is null"));
		return;
	}

	// 2) 전투 카드 풀 구성
	TArray<FName> CombatPool;
	CombatPool.Add(TEXT("Card_GravityField"));
	CombatPool.Add(TEXT("Card_HealingDrone"));
	CombatPool.Add(TEXT("Card_PhaseShift"));
	CombatPool.Add(TEXT("Card_OrbitalStrike"));
	//CombatPool.Add(TEXT("Card_PlasmaOverload"));

	// 3) 카드 시스템 초기화
	CardComp->InitializeCombatCards(CombatPool);

	// 4) (A안) Ability Grant는 Mapping 의존 제거를 위해 비활성화
	//    - 현재 카드 사용(UseSlotCard) 시 TryActivateAbilityByClass를 호출하지만,
	//      "미리 Grant"가 필요하면 다음 단계에서 Grant 로직을 CombatCardComponent로 이관하거나
	//      AbilitySet/AbilityMapping 기반으로 다시 구성한다.
	// GrantCombatCardAbilities();

	UE_LOG(LogTemp, Warning, TEXT("[AuraPS] BeginPlay: CombatPool initialized. (GrantCombatCardAbilities skipped)"));
}

void AAuraPlayerState::GrantCombatCardAbilities()
{
	// (A안) UAuraCombatCardComponent에서 AbilityMappingAsset 접근자를 제거했으므로
	// PlayerState에서 직접 Mapping을 읽어 GiveAbility 하던 구조는 중단.
	// UI/덱/리로드 시스템 마무리 후, 아래 중 하나로 재구성 권장:
	// 1) CombatCardComponent 내부로 Grant 이관 (MappingAsset 소유자가 Grant까지 수행)
	// 2) 별도 AbilitySet(DataAsset) 도입 후 PlayerState는 AbilitySet만 Grant
	UE_LOG(LogTemp, Warning, TEXT("[AuraPS] GrantCombatCardAbilities skipped (Mapping owned by CombatCardComponent)."));
}
