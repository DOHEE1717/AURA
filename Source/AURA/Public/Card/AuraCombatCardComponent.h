#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Card/CardEnum.h"
#include "Card/CardRuntime.h"
#include "UI/AuraCombatHUDTypes.h"
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

	// ===== Core =====
	UFUNCTION(BlueprintCallable, Category="Aura|CombatCard")
	void InitializeCombatCards(const TArray<FName>& InCombatPoolOrder);

	UFUNCTION(BlueprintCallable, Category="Aura|CombatCard")
	bool TryConsumeSlot(int32 SlotIndex, FName& OutCardID);

	UFUNCTION(BlueprintCallable, Category="Aura|CombatCard")
	bool UseSlotCard(int32 SlotIndex, ECardUseInput InputType);
	
	void GrantCombatCardAbilities();

	bool IsCardReady(FName CardID) const;

	// ===== UI Push Delegates =====
	UPROPERTY(BlueprintAssignable, Category="Aura|CombatCard|UI")
	FOnCombatSlotsUIChanged OnCombatSlotsUIChanged;

	UPROPERTY(BlueprintAssignable, Category="Aura|CombatCard|UI")
	FOnCombatReproUIChanged OnCombatReproUIChanged;

	UPROPERTY(BlueprintAssignable, Category="Aura|CombatCard|UI")
	FOnCombatNextUIChanged OnCombatNextUIChanged;

	// HUD Bind 직후 강제 동기화
	UFUNCTION(BlueprintCallable, Category="Aura|CombatCard|UI")
	void BroadcastUI_All();

protected:
	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

private:
	// ===== Ability / Runtime =====
	UAbilitySystemComponent* GetOwnerASC() const;

	UPROPERTY()
	TObjectPtr<UDA_AuraCardAbilityMapping> AbilityMappingAsset;

	UPROPERTY()
	TArray<FName> CombatPoolOrder;

	UPROPERTY()
	TArray<FName> Queue;

	UPROPERTY()
	TArray<FName> Slots;

	UPROPERTY()
	TMap<FName, FCombatCardReproRuntime> ReproTable;

	UPROPERTY(EditAnywhere, Category="Aura|CombatCard|Config")
	int32 MaxSlots = 3;

	UPROPERTY(EditAnywhere, Category="Aura|CombatCard|Config")
	float DefaultReproTime = 2.f;
	
	UPROPERTY()
	TSet<TSubclassOf<UGameplayAbility>> GrantedAbilityClasses;

	TSubclassOf<UGameplayAbility> GetAbilityClassForCard(FName CardID, ECardUseInput InputType) const;

	// ===== Internal =====
	void ResetRuntime();
	void FillInitialSlots();
	void StartReproduction(FName CardID, float ReproTime);
	void UpdateReproduction(float DeltaTime);
	void TryAutoFillSlots();
	bool PopNextReadyFromQueue(FName& OutCardID);

	const UDA_CardDefinition* FindCardDef(FName CardID) const;
	float GetReproTimeFromCardCost(FName CardID) const;

	// ===== UI Snapshot Builders =====
	TArray<FCombatSlotUIData> BuildSlotsUISnapshot() const;
	TArray<FCombatReproUIData> BuildReprosUISnapshot() const;
	FName BuildNextUISnapshot() const;

	// ===== UI Broadcast =====
	void BroadcastUI_Slots();
	void BroadcastUI_Repros();
	void BroadcastUI_Next();

	// Repro 남은시간 갱신용
	float UIReproBroadcastAccum = 0.f;
	float UIReproBroadcastInterval = 0.1f;
	
	bool TryActivateCardAbility(FName CardID, ECardUseInput InputType);
	bool EnsureAbilityGranted(UAbilitySystemComponent* ASC, TSubclassOf<UGameplayAbility> AbilityClass);
};