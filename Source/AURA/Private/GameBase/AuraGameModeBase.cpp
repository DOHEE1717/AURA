// Fill out your copyright notice in the Description page of Project Settings.


#include "GameBase/AuraGameModeBase.h"

#include "GameBase/AuraPlayerController.h"
#include "GameBase/AuraPlayerState.h"
#include "Character/PlayerCharacter.h"


AAuraGameModeBase::AAuraGameModeBase()
{
	// PlayerController → C++
	PlayerControllerClass = AAuraPlayerController::StaticClass();

	// PlayerState → C++
	PlayerStateClass = AAuraPlayerState::StaticClass();

	// Default Pawn → BP 유지
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(
		TEXT("/Game/_BP/Character/BP_PlayerCharacter")
	);

	if (PlayerPawnBPClass.Class)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
}
