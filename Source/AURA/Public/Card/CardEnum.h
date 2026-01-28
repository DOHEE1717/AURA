
#pragma once

#include "CoreMinimal.h"
#include "CardEnum.generated.h"



UENUM(BlueprintType)
enum class ECardType:uint8
{
	None	UMETA(DisplayName="None"),
	Attack UMETA(DisplayName="Attack"),
	Type UMETA(DisplayName="Type"),
	Modifier UMETA(DisplayName="Modifier"),
};

//전투시스템 구성용 
//-Ready : 슬롯에 들어갈 수 있는 카드 상태
//-Reproducing : 재사용 대기시간 
UENUM(BlueprintType)
enum class ECombatCardReproState : uint8
{
	Ready       UMETA(DisplayName="Ready"),
	Reproducing UMETA(DisplayName="Reproducing"),
};
