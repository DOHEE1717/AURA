

#pragma once

#include "CoreMinimal.h"
#include "Character/BaseCharacter.h"
#include "PlayerCharacter.generated.h"

class UCameraComponent;
class USkeletalMeshComponent;
class UInputMappingContext;
class UInputAction;
class UStaticMeshComponent;
struct FInputActionValue;

UCLASS()
class AURA_API APlayerCharacter : public ABaseCharacter
{
	GENERATED_BODY()
	
	//카메라 매시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCamera;
	
	// 1인칭 팔 매시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FirstPerson|Arms", meta = (AllowPrivateAccess = "true"))
	USkeletalMeshComponent* FirstPersonArmsMesh;
	
	// 1인칭 팔 메시에서 숨길 본 목록(팔만 남기기 용도)
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FirstPerson|Arms", meta=(AllowPrivateAccess="true"))
	TArray<FName> ArmsHiddenBones;
	
	// 1인칭 팔 오프셋(카메라 기준) - BP에서 튜닝
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FirstPerson|Arms", meta=(AllowPrivateAccess="true"))
	FVector ArmsRelativeLocation = FVector(35.f, 12.f, -22.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="FirstPerson|Arms", meta=(AllowPrivateAccess="true"))
	FRotator ArmsRelativeRotation = FRotator(0.f, 0.f, 0.f);
	
	//ICM+Action 연결
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input",meta=(AllowPrivateAccess="true"))
	UInputMappingContext* DefaultMappingContext;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input",meta=(AllowPrivateAccess="true"))
	UInputAction* LookAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input",meta=(AllowPrivateAccess="true"))
	UInputAction* MoveAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input",meta=(AllowPrivateAccess="true"))
	UInputAction* RollAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input",meta=(AllowPrivateAccess="true"))
	UInputAction* CrounchAction;
		
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input",meta=(AllowPrivateAccess="true"))
	UInputAction* JumpAction;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Input",meta=(AllowPrivateAccess="true"))
	UInputAction* ZoomAction;
	
public:
	APlayerCharacter();
	
protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	
	
public:
	virtual void Tick(float DeltaTime) override;
	

	
	
	
protected:
	//시점
	void InputLook(const FInputActionValue& Value);
	
	//이동
	void InputMove(const FInputActionValue& Value);
	
	//구르기
	void InputRoll(const FInputActionValue& Value);
	
	//앉기
	void InputCrounchPressed(const FInputActionValue& Value);
	void InputCrounchReleased(const FInputActionValue& Value);
	
	//점프
	void InputJumpPressed(const FInputActionValue& Value);
	void InputJumpReleased(const FInputActionValue& Value);
	
	//줌
	void InputZoomPressed(const FInputActionValue& Value);
	void InputZoomReleased(const FInputActionValue& Value);
	
	//본 숨기기
	void ApplyArmsHiddenBones();
	
};
