// Copyright 2020 Nicholas Chalkley. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "NavigationSystem.h"
#include "GameFramework/Actor.h"
#include "Waypoint.generated.h"

class AWaypointLoop;

UCLASS()
class WAYPOINTS_API AWaypoint : public AActor
{
	GENERATED_UCLASS_BODY()

	virtual void PostRegisterAllComponents() override;
	//virtual void PreEditChange(UProperty* PropertyThatWillChange) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditMove(bool bFinished) override;
	virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;
	//virtual void PostEditImport() override;
	//virtual void PostActorCreated() override;
	//virtual void PostInitProperties() override;
	//virtual void PostInitializeComponents() override;
	//virtual void OnConstruction(const FTransform& Transform) override;
	//virtual void RegisterAllComponents() override;
	//virtual void PostLoad() override;

	virtual bool CanDeleteSelectedActor(FText& OutReason) const override { return true; };
	virtual void BeginDestroy() override;
	virtual void Destroyed() override;

	UFUNCTION()
		AWaypoint* GetNextWaypoint() const;

	UFUNCTION()
		AWaypoint* GetPreviousWaypoint() const;

	UFUNCTION()
		TArray<AWaypoint*> GetLoop() const;

	UFUNCTION()
		float GetWaitTime() const { return WaitTime; };

	UFUNCTION()
		bool GetOrientGuardToWaypoint() const { return bOrientGuardToWaypoint; };

	UFUNCTION()
		bool GetStopOnOverlap() const { return bStopOnOverlap; };

	UFUNCTION()
		float GetAcceptanceRadius() const { return AcceptanceRadius; };

protected:
	bool bHasBeenCopied;

	virtual void RemoveThisWaypoint();

	UPROPERTY()
		class UBillboardComponent* Sprite;

	UPROPERTY()
		class USplineComponent* PathComponent;

	UPROPERTY()
		class UArrowComponent* GuardFacingArrow;

	UPROPERTY(EditInstanceOnly, Category="Waypoint")
		AWaypointLoop* OwningLoop;

	void CalculateSpline();

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
