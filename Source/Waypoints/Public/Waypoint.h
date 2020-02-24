// Copyright 2020 Nicholas Chalkley. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "NavigationSystem.h"
#include "GameFramework/Actor.h"
#include "UObject/WeakObjectPtrTemplates.h"
#include "Waypoint.generated.h"

class AWaypointLoop;

UCLASS()
class WAYPOINTS_API AWaypoint : public AActor
{
	GENERATED_UCLASS_BODY()

	virtual void PostRegisterAllComponents() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditMove(bool bFinished) override;
	virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;

	virtual bool CanDeleteSelectedActor(FText& OutReason) const override { return true; };
	virtual void Destroyed() override;

	UFUNCTION()
		AWaypoint* GetNextWaypoint() const;

	UFUNCTION()
		AWaypoint* GetPreviousWaypoint() const;

	UFUNCTION()
		TArray<TWeakObjectPtr<AWaypoint>> GetLoop() const;

	UFUNCTION()
		float GetWaitTime() const { return WaitTime; };

	UFUNCTION()
		bool GetOrientGuardToWaypoint() const { return bOrientGuardToWaypoint; };

	UFUNCTION()
		bool GetStopOnOverlap() const { return bStopOnOverlap; };

	UFUNCTION()
		float GetAcceptanceRadius() const { return AcceptanceRadius; };

	void CalculateSpline();

protected:
	UPROPERTY()
		TWeakObjectPtr<AWaypoint> WaypointCopiedFrom;

	UPROPERTY()
		class UBillboardComponent* Sprite;

	UPROPERTY()
		class USplineComponent* PathComponent;

	UPROPERTY()
		class UArrowComponent* GuardFacingArrow;

	UPROPERTY(EditInstanceOnly, Category="Waypoint")
		TWeakObjectPtr<AWaypointLoop> OwningLoop;

	UFUNCTION(CallInEditor)
		void OnNavigationGenerationFinished(class ANavigationData* NavData);

protected:
	UFUNCTION(CallInEditor, Category = "Waypoint")
		void SelectNextWaypoint() const;

	UFUNCTION(CallInEditor, Category = "Waypoint")
		void CreateWaypointLoop();

	UPROPERTY()
		class USceneComponent* Scene;

	UPROPERTY(EditDefaultsOnly, Category = "Default")
		FNavAgentProperties NavProperties;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default", meta = (ClampMin = "0.0", UIMin = "0.0"))
		float WaitTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
		bool bOrientGuardToWaypoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default")
		bool bStopOnOverlap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Default", meta = (ClampMin = "-1.0", UIMin = "-1.0"))
		float AcceptanceRadius;
};
