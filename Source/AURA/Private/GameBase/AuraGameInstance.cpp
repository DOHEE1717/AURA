// Fill out your copyright notice in the Description page of Project Settings.


#include "GameBase/AuraGameInstance.h"

void UAuraGameInstance::Init()
{
	Super::Init();
	
	
	UE_LOG(LogTemp, Warning, TEXT("[GI] BuildCardRegistry DONE. Count=%d"), CardRegistry.Num());
	
}

void UAuraGameInstance::OnStart()
{
	Super::OnStart();

	UE_LOG(LogTemp, Warning, TEXT("[GI] OnStart() called. Class=%s"), *GetNameSafe(GetClass()));

	BuildCardRegistry();
	UE_LOG(LogTemp, Warning, TEXT("[GI] OnStart() BuildCardRegistry DONE. Count=%d"), CardRegistry.Num());
}

void UAuraGameInstance::BuildCardRegistry()
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
