// Fill out your copyright notice in the Description page of Project Settings.


#include "GameBase/GameInstance_Aura.h"

void UGameInstance_Aura::Init()
{
	Super::Init();
}

void UGameInstance_Aura::BuildCardRegistry()
{
	CardRegistry.Reset();
	
	for (const TObjectPtr<UDA_CardDefinition>&Def:AllCardDefinitions)
	{
		if (!Def)
		{
			continue;
		}
		
		const FName Key = Def->CardID;
		
		if (Key.IsNone())
		{
			UE_LOG(LogTemp, Warning, TEXT("[BuildCardRegistry] CardID is None. Asset: %s"), *Def->GetName());
			continue;
		}
		
		if (CardRegistry.Contains(Key))
		{
			UE_LOG(LogTemp, Warning, TEXT("[BuildCardRegistry] Duplicate CardID '%s'. Asset: %s"),
				*Key.ToString(), *Def->GetName());
			continue;
		}
		
		CardRegistry.Add(Key, Def);
			
	}
	UE_LOG(LogTemp, Log, TEXT("[BuildCardRegistry] Built: %d cards"), CardRegistry.Num());
}
