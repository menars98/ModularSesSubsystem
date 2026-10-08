// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "OnlineSubsystem.h" 
#include "OnlineSessionSettings.h"   
#include "Interfaces/OnlineSessionInterface.h"
#include "FindSessionsCallbackProxy.h"
#include "MSSSessionSubsystem.generated.h"

// Delegates
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMSSOnCreateSessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FMSSOnFindSessionsComplete, const TArray<FBlueprintSessionResult>&, SessionResults, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMSSOnJoinSessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMSSOnDestroySessionComplete, bool, bWasSuccessful);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMSSOnStartSessionComplete, bool, bWasSuccessful);

/**
 * MSS (Modular Sessions Subsystem) 
 */
UCLASS()
class MULTIPLAYERSESSIONS_API UMSSSessionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()
	
public:

	UMSSSessionSubsystem();

	// Lifecycle Functions
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// ----------------------------------------------------------------------------------
	// Public API Functions
	// ----------------------------------------------------------------------------------
	UFUNCTION(BlueprintCallable, Category = "MSS|Sessions")
	void CreateSession(int32 NumPublicConnections = -1, bool bIsLAN = false, FString MatchType = TEXT(""));

	UFUNCTION(BlueprintCallable, Category = "MSS|Sessions")
	void FindSessions(int32 MaxSearchResults = -1);

	UFUNCTION(BlueprintCallable, Category = "MSS|Sessions")
	void JoinSession(const FBlueprintSessionResult& SessionResult);

	UFUNCTION(BlueprintCallable, Category = "MSS|Sessions")
	void DestroySession();

	UFUNCTION(BlueprintCallable, Category = "MSS|Sessions")
	void StartSession();

	// ----------------------------------------------------------------------------------
	// Event Dispatchers
	// ----------------------------------------------------------------------------------
	UPROPERTY(BlueprintAssignable, Category = "MSS|Events")
	FMSSOnCreateSessionComplete OnSessionCreateComplete;

	UPROPERTY(BlueprintAssignable, Category = "MSS|Events")
	FMSSOnFindSessionsComplete	 OnSessionFindComplete;

	UPROPERTY(BlueprintAssignable, Category = "MSS|Events")
	FMSSOnJoinSessionComplete OnSessionJoinComplete;

	UPROPERTY(BlueprintAssignable, Category = "MSS|Events")
	FMSSOnDestroySessionComplete OnSessionDestroyComplete;

	UPROPERTY(BlueprintAssignable, Category = "MSS|Events")
	FMSSOnStartSessionComplete OnSessionStartComplete;

protected:
	// Internal asynchronous callbacks
	void HandleCreateSessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleFindSessionsComplete(bool bWasSuccessful);
	void HandleJoinSessionComplete(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void HandleDestroySessionComplete(FName SessionName, bool bWasSuccessful);
	void HandleStartSessionComplete(FName SessionName, bool bWasSuccessful);

private:
	IOnlineSessionPtr GetSessionInterface();

	IOnlineSessionPtr SessionInterface;
	TSharedPtr<FOnlineSessionSettings> LastSessionSettings;
	TSharedPtr<FOnlineSessionSearch> LastSessionSearch;

	// Delege Tanýmlayýcýlarý
	FDelegateHandle CreateSessionDelegateHandle;
	FDelegateHandle FindSessionsDelegateHandle;
	FDelegateHandle JoinSessionDelegateHandle;
	FDelegateHandle DestroySessionDelegateHandle;
	FDelegateHandle StartSessionDelegateHandle;

	// Güvenlik ve durum deðiþkenleri
	bool bCreateSessionOnDestroy = false;
	int32 LastNumPublicConnections = 4;
	bool bLastIsLAN = false;
	FString LastMatchType = TEXT("");
};
