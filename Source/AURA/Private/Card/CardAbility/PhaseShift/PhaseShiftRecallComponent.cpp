#include "Card/CardAbility/PhaseShift/PhaseShiftRecallComponent.h"

#include "GameFramework/Actor.h"
#include "Engine/World.h"

UPhaseShiftRecallComponent::UPhaseShiftRecallComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

bool UPhaseShiftRecallComponent::HasSavedLocation() const
{
	return bHasSaved && !IsExpired();
}

void UPhaseShiftRecallComponent::SaveCurrentLocation(const AActor* ActorToReadFrom)
{
	if (!ActorToReadFrom) return;

	bHasSaved = true;
	SavedLocation = ActorToReadFrom->GetActorLocation();
	SavedRotation = ActorToReadFrom->GetActorRotation();

	if (UWorld* World = GetWorld())
	{
		SavedWorldTime = World->GetTimeSeconds();
	}
	else
	{
		SavedWorldTime = 0.f;
	}
}

bool UPhaseShiftRecallComponent::RecallToSavedLocation(AActor* ActorToMove, bool bClearAfterRecall)
{
	if (!ActorToMove) return false;
	if (!HasSavedLocation()) return false;

	ActorToMove->TeleportTo(SavedLocation, SavedRotation, false, true);

	if (bClearAfterRecall)
	{
		ClearSavedLocation();
	}
	return true;
}

void UPhaseShiftRecallComponent::ClearSavedLocation()
{
	bHasSaved = false;
	SavedLocation = FVector::ZeroVector;
	SavedRotation = FRotator::ZeroRotator;
	SavedWorldTime = 0.f;
}

void UPhaseShiftRecallComponent::SetExpireSeconds(float InSeconds)
{
	ExpireSeconds = FMath::Max(0.f, InSeconds);
}

bool UPhaseShiftRecallComponent::IsExpired() const
{
	if (!bHasSaved) return false;
	if (ExpireSeconds <= 0.f) return false;

	UWorld* World = GetWorld();
	if (!World) return false;

	const float Now = World->GetTimeSeconds();
	return (Now - SavedWorldTime) >= ExpireSeconds;
}