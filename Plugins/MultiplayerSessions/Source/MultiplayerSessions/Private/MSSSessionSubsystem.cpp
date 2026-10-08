// Fill out your copyright notice in the Description page of Project Settings.

#include "MSSSessionSubsystem.h"
#include "MSSModularSessionsSettings.h"
#include "OnlineSubsystemUtils.h"
#include "Kismet/GameplayStatics.h"
#include "OnlineSessionSettings.h"
#include "MSSSessionSubsystem.h"

UMSSSessionSubsystem::UMSSSessionSubsystem()
{
}

void UMSSSessionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	GetSessionInterface();
}

void UMSSSessionSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

IOnlineSessionPtr UMSSSessionSubsystem::GetSessionInterface()
{
	if (!SessionInterface.IsValid())
	{
		IOnlineSubsystem* Subsystem = IOnlineSubsystem::Get();
		if (Subsystem)
		{
			SessionInterface = Subsystem->GetSessionInterface();
		}
	}
	return SessionInterface;
}

// ----------------------------------------------------------------------------------
// 1. CREATE SESSION
// ----------------------------------------------------------------------------------
void UMSSSessionSubsystem::CreateSession(int32 NumPublicConnections, bool bIsLAN, FString MatchType)
{
	if (!GetSessionInterface().IsValid())
	{
		OnSessionCreateComplete.Broadcast(false);
		return;
	}

	// Check if we have default settings for NumPublicConnections and MatchType, and if not, use the defaults from the settings asset
	const UMSSModularSessionsSettings* Settings = GetDefault<UMSSModularSessionsSettings>();
	if (NumPublicConnections <= 0)
	{
		NumPublicConnections = Settings ? Settings->DefaultMaxNumPlayers : 4;
	}
	if (MatchType.IsEmpty())
	{
		MatchType = Settings ? Settings->DefaultMatchType : TEXT("FreeForAll");
	}

	// If there is already an existing session, destroy it first before creating a new one (Safety Check)
	auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
	if (ExistingSession != nullptr)
	{
		bCreateSessionOnDestroy = true;
		LastNumPublicConnections = NumPublicConnections;
		bLastIsLAN = bIsLAN;
		LastMatchType = MatchType;

		DestroySession();
		return;
	}

	// Bind the delegate and store the handle
	CreateSessionDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &UMSSSessionSubsystem::HandleCreateSessionComplete)
	);

	// Configure session settings
	LastSessionSettings = MakeShareable(new FOnlineSessionSettings());
	LastSessionSettings->bIsLANMatch = bIsLAN;
	LastSessionSettings->NumPublicConnections = NumPublicConnections;
	LastSessionSettings->bAllowJoinInProgress = true;
	LastSessionSettings->bAllowJoinViaPresence = true;
	LastSessionSettings->bShouldAdvertise = true;
	LastSessionSettings->bUsesPresence = true;
	LastSessionSettings->bUseLobbiesIfAvailable = true;

	// Add a custom MatchType key to filter the room
	LastSessionSettings->Set(FName("MSS_MATCH_TYPE"), MatchType, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!LocalPlayer || !SessionInterface->CreateSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, *LastSessionSettings))
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionDelegateHandle);
		OnSessionCreateComplete.Broadcast(false);
	}
}

void UMSSSessionSubsystem::HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionDelegateHandle);
	}

	// Notify Blueprint and UI listeners
	OnSessionCreateComplete.Broadcast(bWasSuccessful);
}

// ----------------------------------------------------------------------------------
// 2. FIND SESSIONS
// ----------------------------------------------------------------------------------
void UMSSSessionSubsystem::FindSessions(int32 MaxSearchResults)
{
	if (!GetSessionInterface().IsValid())
	{
		OnSessionFindComplete.Broadcast(TArray<FBlueprintSessionResult>(), false);
		return;
	}

	const UMSSModularSessionsSettings* Settings = GetDefault<UMSSModularSessionsSettings>();
	if (MaxSearchResults <= 0)
	{
		MaxSearchResults = Settings ? Settings->MaxSearchResults : 100;
	}

	FindSessionsDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &UMSSSessionSubsystem::HandleFindSessionsComplete)
	);


	LastSessionSearch = MakeShareable(new FOnlineSessionSearch());
	LastSessionSearch->MaxSearchResults = MaxSearchResults;
	const bool bIsLAN = (IOnlineSubsystem::Get()->GetSubsystemName() == "NULL");
	LastSessionSearch->bIsLanQuery = bIsLAN;
	// Only perform the presence query if it's internet/Steam, never for LAN!
	if (!bIsLAN)
	{
		LastSessionSearch->QuerySettings.Set(FName(TEXT("PRESENCESEARCH")), true, EOnlineComparisonOp::Equals);
	}

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!LocalPlayer || !SessionInterface->FindSessions(*LocalPlayer->GetPreferredUniqueNetId(), LastSessionSearch.ToSharedRef()))
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsDelegateHandle);
		OnSessionFindComplete.Broadcast(TArray<FBlueprintSessionResult>(), false);
	}
}

void UMSSSessionSubsystem::HandleFindSessionsComplete(bool bWasSuccessful)
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsDelegateHandle);
	}

	TArray<FBlueprintSessionResult> OutResults;

	if (bWasSuccessful && LastSessionSearch.IsValid())
	{
		// Convert C++ results to FBlueprintSessionResult that UMG interfaces can read
		for (const FOnlineSessionSearchResult& SearchResult : LastSessionSearch->SearchResults)
		{
			FBlueprintSessionResult BPResult;
			BPResult.OnlineResult = SearchResult;
			OutResults.Add(BPResult);
		}
	}

	OnSessionFindComplete.Broadcast(OutResults, bWasSuccessful);
}

// ----------------------------------------------------------------------------------
// 3. JOIN SESSION
// ----------------------------------------------------------------------------------
void UMSSSessionSubsystem::JoinSession(const FBlueprintSessionResult& SessionResult)
{
	if (!GetSessionInterface().IsValid())
	{
		OnSessionJoinComplete.Broadcast(false);
		return;
	}

	JoinSessionDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &UMSSSessionSubsystem::HandleJoinSessionComplete)
	);

	const ULocalPlayer* LocalPlayer = GetWorld()->GetFirstLocalPlayerFromController();
	if (!LocalPlayer || !SessionInterface->JoinSession(*LocalPlayer->GetPreferredUniqueNetId(), NAME_GameSession, SessionResult.OnlineResult))
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionDelegateHandle);
		OnSessionJoinComplete.Broadcast(false);
	}
}

void UMSSSessionSubsystem::HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionDelegateHandle);
	}

	bool bSuccess = (Result == EOnJoinSessionCompleteResult::Success);

	if (bSuccess)
	{
		// If the join was successful, resolve the IP address and travel the player using ClientTravel
		FString ConnectString;
		if (SessionInterface->GetResolvedConnectString(NAME_GameSession, ConnectString))
		{
			APlayerController* PlayerController = GetGameInstance()->GetFirstLocalPlayerController();
			if (PlayerController)
			{
				PlayerController->ClientTravel(ConnectString, ETravelType::TRAVEL_Absolute);
			}
		}
	}

	OnSessionJoinComplete.Broadcast(bSuccess);
}

// ----------------------------------------------------------------------------------
// 4. DESTROY SESSION
// ----------------------------------------------------------------------------------
void UMSSSessionSubsystem::DestroySession()
{
	if (!GetSessionInterface().IsValid())
	{
		OnSessionDestroyComplete.Broadcast(false);
		return;
	}

	DestroySessionDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
		FOnDestroySessionCompleteDelegate::CreateUObject(this, &UMSSSessionSubsystem::HandleDestroySessionComplete)
	);

	if (!SessionInterface->DestroySession(NAME_GameSession))
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionDelegateHandle);
		OnSessionDestroyComplete.Broadcast(false);
	}
}

void UMSSSessionSubsystem::HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionDelegateHandle);
	}

	// If this destroy operation was triggered to create a new room, create a new one
	if (bWasSuccessful && bCreateSessionOnDestroy)
	{
		bCreateSessionOnDestroy = false;
		CreateSession(LastNumPublicConnections, bLastIsLAN, LastMatchType);
		return;
	}

	OnSessionDestroyComplete.Broadcast(bWasSuccessful);
}

// ----------------------------------------------------------------------------------
// 5. START SESSION
// ----------------------------------------------------------------------------------
void UMSSSessionSubsystem::StartSession()
{
	if (!GetSessionInterface().IsValid())
	{
		OnSessionStartComplete.Broadcast(false);
		return;
	}

	StartSessionDelegateHandle = SessionInterface->AddOnStartSessionCompleteDelegate_Handle(
		FOnStartSessionCompleteDelegate::CreateUObject(this, &UMSSSessionSubsystem::HandleStartSessionComplete)
	);

	if (!SessionInterface->StartSession(NAME_GameSession))
	{
		SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionDelegateHandle);
		OnSessionStartComplete.Broadcast(false);
	}
}

void UMSSSessionSubsystem::HandleStartSessionComplete(FName SessionName, bool bWasSuccessful)
{
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnStartSessionCompleteDelegate_Handle(StartSessionDelegateHandle);
	}

	OnSessionStartComplete.Broadcast(bWasSuccessful);
}