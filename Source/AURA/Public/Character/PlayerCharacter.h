

#pragma once

#include "CoreMinimal.h"
#include "Character/BaseCharacter.h"
#include "PlayerCharacter.generated.h"

class UCameraComponent;
class UInputMappingContext;
class UInputAction;
class UStaticMeshComponent;
struct FInputActionValue;
class UAbilitySystemComponent;

UCLASS()
class AURA_API APlayerCharacter : public ABaseCharacter
{
	GENERATED_BODY()
	
	//ICM+Action 연결
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera", meta = (AllowPrivateAccess = "true"))
	UCameraComponent* FirstPersonCamera;
	
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
	
public:
	UAbilitySystemComponent* GetASC() const;
	
	UFUNCTION(BlueprintCallable, Category="GAS")
	UAbilitySystemComponent* GetASC_BP() const;
	

	
	
	
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
	
};
