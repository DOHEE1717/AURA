// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Card/CardEnum.h"
#include "Card/CardRuntime.h"
#include "AuraCombatCardComponent.generated.h"


class UDA_CardDefinition;
class UDA_AuraCardAbilityMapping;
class UGameplayAbility;
class UAbilitySystemComponent;

UENUM(BlueprintType)
enum class ECardUseInput : uint8
{
	Primary,
	Alt
};


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class AURA_API UAuraCombatCardComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAuraCombatCardComponent();

	//커드 풀 순서로 초기화 , 슬록 채우기
	UFUNCTION(BlueprintCallable, Category="Aura|CombatCard")
	void InitializeCombatCards(const TArray<FName>& InCombatPoolOrder);

	//카드 사용함수
	UFUNCTION(BlueprintCallable, Category="Aura|CombatCard")
	bool TryConsumeSlot(int32 SlotIndex, FName& OutCardID);

	//UI 제작용
	UFUNCTION(BlueprintCallable, Category="Aura|CombatCard")
	void GetSlots(TArray<FName>& OutSlots) const;

	UFUNCTION(BlueprintCallable, Category="Aura|CombatCard")
	void GetQueue(TArray<FName>& OutQueue) const;

	UFUNCTION(BlueprintCallable, Category="Aura|CombatCard")
	void GetReproducingCards(TArray<FCombatCardReproRuntime>& OutReproducing) const;

	UFUNCTION(BlueprintCallable, Category="Aura|CombatCard")
	bool IsCardReady(FName CardID) const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Aura|CombatCard|Debug")
	bool bDebugPrint = false;
	
	//매핑에서 pri/alt ability 실행. 성공시 소비,재생산 처리용 함수
	UFUNCTION(BlueprintCallable, Category="Card|Ability")
	bool UseSlotCard(int32 SlotIndex, ECardUseInput InputType);
	
	// 매핑 에셋 getter
	const UDA_AuraCardAbilityMapping* GetAbilityMappingAsset() const { return AbilityMappingAsset; }

	// 전투 카드 풀 getter (InitializeCombatCards로 들어온 풀)
	const TArray<FName>& GetCombatPoolOrder() const { return CombatPoolOrder; }


protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;


	// Ability 매핑 에셋(에디터에서 DA_CardAbilityMapping_Default 지정)
	UPROPERTY(EditDefaultsOnly, Category="Card|Ability")
	TObjectPtr<UDA_AuraCardAbilityMapping> AbilityMappingAsset;

	
private:
	// ASC 얻기(Owner=PlayerState 전제)
	UAbilitySystemComponent* GetOwnerASC() const;
	
	// 전투 카드 풀
	UPROPERTY(VisibleAnywhere, Category="Aura|CombatCard|Runtime")
	TArray<FName> CombatPoolOrder;

	// 재생산 대기열
	UPROPERTY(VisibleAnywhere, Category="Aura|CombatCard|Runtime")
	TArray<FName> Queue;

	// 슬롯 3칸 
	UPROPERTY(VisibleAnywhere, Category="Aura|CombatCard|Runtime")
	TArray<FName> Slots;

	// 카드별 재생산 상태 테이블
	UPROPERTY(VisibleAnywhere, Category="Aura|CombatCard|Runtime")
	TMap<FName, FCombatCardReproRuntime> ReproTable;

	UPROPERTY(EditAnywhere, Category="Aura|CombatCard|Config")
	int32 MaxSlots = 3;

	// 카드별 시간이 생기기 전까지는 기본값으로 사용
	UPROPERTY(EditAnywhere, Category="Aura|CombatCard|Config")
	float DefaultReproTime = 2.0f;

	const UDA_CardDefinition* FindCardDef(FName CardID) const;
	float GetReproTimeFromCardCost(FName CardID) const;

	/* ===== Internal Helpers ===== */

	void ResetRuntime();
	void FillInitialSlots();

	void StartReproduction(FName CardID, float ReproTime);
	void UpdateReproduction(float DeltaTime);

	void TryAutoFillSlots();
	bool PopNextReadyFromQueue(FName& OutCardID);

	void DebugDumpState(const TCHAR* Context) const;
};
