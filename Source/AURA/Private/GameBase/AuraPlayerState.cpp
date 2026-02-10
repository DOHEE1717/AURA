// Fill out your copyright notice in the Description page of Project Settings.

#include "GameBase/AuraPlayerState.h"

#include "AbilitySystemComponent.h"
#include "GameBase/AuraAttributeSet.h"

#include "Card/AuraCombatCardComponent.h"
#include "Card/DA_AuraCardAbilityMapping.h"
#include "Abilities/GameplayAbility.h"
#include "Card/CardAbility/OrbitalStrike/OrbitalReconComponent.h"
#include "Card/CardAbility/PhaseShift/PhaseShiftRecallComponent.h"

AAuraPlayerState::AAuraPlayerState()
{
	bReplicates = true;

	// ASC
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// ✅ AttributeSet (이 줄이 핵심)
	AttributeSet = CreateDefaultSubobject<UAuraAttributeSet>(TEXT("AuraAttributeSet"));

	// Combat Card Component
	CombatCardComponent = CreateDefaultSubobject<UAuraCombatCardComponent>(TEXT("CombatCardComponent"));

	// Orbital Recon
	OrbitalReconComp = CreateDefaultSubobject<UOrbitalReconComponent>(TEXT("OrbitalReconComp"));
	
	//PhaseShift
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
	CombatPool.Add(TEXT("Card_PlasmaOverload"));

	// 3) 카드 시스템 초기화
	CardComp->InitializeCombatCards(CombatPool);

	// 4) 초기화 직후 Ability 부여
	GrantCombatCardAbilities();
}

void AAuraPlayerState::GrantCombatCardAbilities()
{
	// 서버에서만 Ability 부여
	if (!HasAuthority())
	{
		return;
	}

	UAbilitySystemComponent* ASC = AbilitySystemComponent;
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AuraPS] GrantCombatCardAbilities: ASC is null"));
		return;
	}

	UAuraCombatCardComponent* CardComp = GetCombatCardComponent();
	if (!CardComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AuraPS] GrantCombatCardAbilities: CombatCardComponent is null"));
		return;
	}

	const UDA_AuraCardAbilityMapping* Mapping = CardComp->GetAbilityMappingAsset();
	if (!Mapping)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[AuraPS] GrantCombatCardAbilities: AbilityMappingAsset is null"));
		return;
	}

	const TArray<FName>& Pool = CardComp->GetCombatPoolOrder();
	if (Pool.Num() == 0)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[AuraPS] GrantCombatCardAbilities: CombatPoolOrder is empty"));
		return;
	}

	int32 GrantedCount = 0;

	for (const FName& CardID : Pool)
	{
		if (CardID.IsNone())
		{
			continue;
		}

		FCardAbilityPair Pair;
		if (!Mapping->GetAbilityPair(CardID, Pair))
		{
			continue;
		}

		auto GiveIfNeeded = [&](TSubclassOf<UGameplayAbility> AbilityClass)
		{
			if (!AbilityClass) return;

			if (ASC->FindAbilitySpecFromClass(AbilityClass))
			{
				return;
			}

			ASC->GiveAbility(FGameplayAbilitySpec(AbilityClass, 1, INDEX_NONE));
			GrantedCount++;
		};

		GiveIfNeeded(Pair.PrimaryAbility);
		GiveIfNeeded(Pair.AltAbility);
	}

	UE_LOG(LogTemp, Warning,
		TEXT("[AuraPS] GrantCombatCardAbilities: Granted=%d (Pool=%d)"),
		GrantedCount, Pool.Num());
}
