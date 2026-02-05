// Fill out your copyright notice in the Description page of Project Settings.


#include "GameBase/AuraPlayerState.h"
#include "AbilitySystemComponent.h"
#include "Card/AuraCombatCardComponent.h"
#include "Card/DA_AuraCardAbilityMapping.h"
#include "Abilities/GameplayAbility.h"
#include "Card/CardAbility/OrbitalStrike/OrbitalReconComponent.h"




AAuraPlayerState::AAuraPlayerState()
{
	bReplicates=true;
	
	AbilitySystemComponent=CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	
	//CardCombatComponent 추가
	CombatCardComponent=CreateDefaultSubobject<UAuraCombatCardComponent>(TEXT("CombatCardComponent"));
	
	OrbitalReconComp = CreateDefaultSubobject<UOrbitalReconComponent>(TEXT("OrbitalReconComp"));
	
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

	// 2) 전투 카드 풀 구성(하드코딩
	TArray<FName> CombatPool;
	CombatPool.Add(TEXT("Card_GravityField"));
	CombatPool.Add(TEXT("Card_HealingDrone"));
	CombatPool.Add(TEXT("Card_OrbitalStrike"));
	CombatPool.Add(TEXT("Card_PhaseShift"));
	CombatPool.Add(TEXT("Card_PlasmaOverload"));

	// 3) 카드 시스템 초기화
	CardComp->InitializeCombatCards(CombatPool);

	// 4) (중요) 초기화 직후 Ability들 Give
	GrantCombatCardAbilities();
}


void AAuraPlayerState::GrantCombatCardAbilities()
{
	// GAS Ability 부여는 서버 권장
	if (!HasAuthority())
	{
		return;
	}

	// ASC 얻기 (네 프로젝트에서 PS가 ASC 보유한다고 했으니)
	UAbilitySystemComponent* ASC = FindComponentByClass<UAbilitySystemComponent>();
	if (!ASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AuraPS] GrantCombatCardAbilities: ASC is null"));
		return;
	}

	UAuraCombatCardComponent* CardComp = GetCombatCardComponent(); // 네가 이미 갖고있는 getter
	if (!CardComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AuraPS] GrantCombatCardAbilities: CombatCardComponent is null"));
		return;
	}

	const UDA_AuraCardAbilityMapping* Mapping = CardComp->GetAbilityMappingAsset();
	if (!Mapping)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AuraPS] GrantCombatCardAbilities: AbilityMappingAsset is null (set DA_CardAbilityMapping_Default on component)"));
		return;
	}

	const TArray<FName>& Pool = CardComp->GetCombatPoolOrder();
	if (Pool.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[AuraPS] GrantCombatCardAbilities: CombatPoolOrder is empty (InitializeCombatCards not called yet?)"));
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
			// 카드풀엔 있는데 매핑이 비어있을 수 있음(일단 스킵)
			continue;
		}

		auto GiveIfNeeded = [&](TSubclassOf<UGameplayAbility> AbilityClass)
		{
			if (!AbilityClass) return;

			// 이미 부여된 경우 중복 방지
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

	UE_LOG(LogTemp, Warning, TEXT("[AuraPS] GrantCombatCardAbilities: Granted=%d (Pool=%d)"), GrantedCount, Pool.Num());
}
