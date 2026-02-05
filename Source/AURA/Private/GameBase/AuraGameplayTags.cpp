#include "GameBase/AuraGameplayTags.h"
#include "GameplayTagsManager.h"

const FGameplayTag TAG_TargetingActive =
	UGameplayTagsManager::Get().RequestGameplayTag(FName("State.Targeting.Active"));
