// Fill out your copyright notice in the Description page of Project Settings.


#include "Character/PlayerCharacter.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"  
#include "GameFramework/CharacterMovementComponent.h"

//입력관련헤더
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

APlayerCharacter::APlayerCharacter()
{
	PrimaryActorTick.bCanEverTick=true;
	
	//바디생성
	GetMesh()->SetRelativeLocation(FVector(0.f, 0.f, -90.f));
	GetMesh()->SetRelativeRotation(FRotator(0.f, -90.f, 0.f));
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	//화면에서 바디 숨기고, 그림자만 생성
	GetMesh()->SetOwnerNoSee(true);
	GetMesh()->SetCastHiddenShadow(true);
		
	
	//1인칭 카메라 생성
	FirstPersonCamera=CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCamera->SetupAttachment(GetCapsuleComponent());
	FirstPersonCamera->SetRelativeLocation(FVector(0.0f, 0.0f, 64.0f));
	FirstPersonCamera->bUsePawnControlRotation=true;
	
	// 1인칭 팔 메시 생성 
	FirstPersonArmsMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FirstPersonArmsMesh"));
	FirstPersonArmsMesh->SetupAttachment(FirstPersonCamera);

	// 충돌은 비활성화
	FirstPersonArmsMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// 팔은 1인칭에서만 보이게
	FirstPersonArmsMesh->SetOnlyOwnerSee(true);
	FirstPersonArmsMesh->bOwnerNoSee = false;

	// 팔 그림자 비활성화
	FirstPersonArmsMesh->CastShadow = false;
	FirstPersonArmsMesh->bCastDynamicShadow = false;
	
	//컨트롤러 회전사용 
	bUseControllerRotationPitch=true;
	bUseControllerRotationYaw=true;
	bUseControllerRotationRoll=true;
	
	//이동시 자동회전 끄기
	if(UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->bOrientRotationToMovement=false;
	}
}

void APlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	
	//IMC를 캐릭터에 적용
	if (APlayerController* PlayerController = Cast<APlayerController>(GetController()))
	{
		if (ULocalPlayer* Localplayer=PlayerController->GetLocalPlayer())
		{
			if (UEnhancedInputLocalPlayerSubsystem* Subsys = Localplayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
			{
				if (DefaultMappingContext)
				{
					Subsys->AddMappingContext(DefaultMappingContext, 0);
				}
				
			}
		}
	}
	
	ApplyArmsHiddenBones();
		
	GetMesh()->SetVisibility(false, true);
	GetMesh()->SetHiddenInGame(true, true);
	
	// 팔 오프셋 적용(카메라 기준)
	if (FirstPersonArmsMesh)
	{
		FirstPersonArmsMesh->SetRelativeLocation(ArmsRelativeLocation);
		FirstPersonArmsMesh->SetRelativeRotation(ArmsRelativeRotation);
	}
}

void APlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
		
	
	//EnhancedInput 캐스팅
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EnhancedInputComponent) return;
	
	//시점이동
	if (LookAction)
	{
		EnhancedInputComponent->BindAction(LookAction,ETriggerEvent::Triggered,this,&APlayerCharacter::InputLook);
	}
	
	//이동
	if (MoveAction)
	{
		EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APlayerCharacter::InputMove);
	}
	
	//구르기
	if (RollAction)
	{
		EnhancedInputComponent->BindAction(RollAction, ETriggerEvent::Started, this, &APlayerCharacter::InputRoll);
	}
	
	//앉기
	if (CrounchAction)
	{
		EnhancedInputComponent->BindAction(CrounchAction,ETriggerEvent::Started, this, &APlayerCharacter::InputCrounchPressed);
		EnhancedInputComponent->BindAction(CrounchAction,ETriggerEvent::Completed, this, &APlayerCharacter::InputCrounchReleased);
	}
	
	//점프
	if (JumpAction)
	{
		EnhancedInputComponent->BindAction(JumpAction,ETriggerEvent::Started, this, &APlayerCharacter::InputJumpPressed);
		EnhancedInputComponent->BindAction(JumpAction,ETriggerEvent::Canceled, this, &APlayerCharacter::InputJumpReleased);
		
	}
	
	//줌, 기능 구현만 해두고 안쓸지도
	if (ZoomAction)
	{
		EnhancedInputComponent->BindAction(ZoomAction,ETriggerEvent::Started, this, &APlayerCharacter::InputZoomPressed);
		EnhancedInputComponent->BindAction(ZoomAction,ETriggerEvent::Canceled, this, &APlayerCharacter::InputZoomReleased);
		
	}
		
	
}


void APlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void APlayerCharacter::InputLook(const FInputActionValue& Value)
{const FVector2D LookAxis=Value.Get<FVector2D>();
	
	//마우스 X는 Yaw 로 
	AddControllerYawInput(LookAxis.X);
	//마우스 Y는 Pitch 로
	AddControllerPitchInput(LookAxis.Y);
}

void APlayerCharacter::InputMove(const FInputActionValue& Value)
{
	
	
	const FVector2D MoveAxis=Value.Get<FVector2D>();
		
	if (!Controller)return;
	
	const FRotator Rotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.f, Rotation.Yaw, 0.f);

	const FVector Forward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector Right   = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	
	AddMovementInput(Forward,MoveAxis.X);
	AddMovementInput(Right,MoveAxis.Y);	
	
}

void APlayerCharacter::InputRoll(const FInputActionValue& Value)
{
	
}

void APlayerCharacter::InputCrounchPressed(const FInputActionValue& Value)
{
	Crouch();
}

void APlayerCharacter::InputCrounchReleased(const FInputActionValue& Value)
{
	UnCrouch();
}

void APlayerCharacter::InputJumpPressed(const FInputActionValue& Value)
{
	Jump();
}

void APlayerCharacter::InputJumpReleased(const FInputActionValue& Value)
{
	StopJumping();
}

void APlayerCharacter::InputZoomPressed(const FInputActionValue& Value)
{

}

void APlayerCharacter::InputZoomReleased(const FInputActionValue& Value)
{
	
}

void APlayerCharacter::ApplyArmsHiddenBones()
{
	if (!FirstPersonArmsMesh) return;
	
	if (ArmsHiddenBones.Num() == 0)
	{
		
		ArmsHiddenBones = {
			// === 머리/목 체인 ===
			TEXT("neck_01"),
			
			// === 하체 체인 (좌) ===
			TEXT("thigh_l"),
			
			// === 하체 체인 (우) ===
			TEXT("thigh_r")
			
		};
	}
	
	// 숨김 적용
	for (const FName& BoneName : ArmsHiddenBones)
	{
		if (BoneName.IsNone()) continue;

		// 물리 바디 처리는 건드리지 않음(PBO_None)
		FirstPersonArmsMesh->HideBoneByName(BoneName, EPhysBodyOp::PBO_None);
	}
	
}
