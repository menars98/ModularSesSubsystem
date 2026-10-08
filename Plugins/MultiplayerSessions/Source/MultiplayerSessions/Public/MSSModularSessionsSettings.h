// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MSSModularSessionsSettings.generated.h"

/**
 * Settings that will appear under Project Settings -> Plugins -> Modular Sessions
 */
UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Modular Sessions"))
class MULTIPLAYERSESSIONS_API UMSSModularSessionsSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
	UMSSModularSessionsSettings();

	/** Which category it will appear under in the project settings */
	virtual FName GetCategoryName() const override { return FName("Plugins"); }

	/** Default maximum number of players per room */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "General", meta = (ClampMin = "1", ClampMax = "100"))
	int32 DefaultMaxNumPlayers = 4;

	/** Default maximum number of rooms to search for */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "General", meta = (ClampMin = "1", ClampMax = "500"))
	int32 MaxSearchResults = 100;

	/** Whether to default to LAN for easier testing */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "General")
	bool bDefaultToLAN = true;

	/** Default key for filtering rooms by game mode/type in matchmaking */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "Matchmaking")
	FString DefaultMatchType = TEXT("FreeForAll");
};
