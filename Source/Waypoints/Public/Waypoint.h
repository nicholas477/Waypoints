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
#if WITH_EDITOR
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditMove(bool bFinished) override;
	virtual bool CanDeleteSelectedActor(FText& OutReason) const override { return true; };
#endif // WITH_EDITOR
	virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;
	virtual void Destroyed() override;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Waypoint")
		AWaypoint* GetNextWaypoint() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Waypoint")
		AWaypoint* GetPreviousWaypoint() const;

	UFUNCTION()
		TArray<TWeakObjectPtr<AWaypoint>> GetLoop() const;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Waypoint")
		float GetWaitTime() const { return WaitTime; };

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Waypoint")
		bool GetOrientGuardToWaypoint() const { return bOrientGuardToWaypoint; };

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Waypoint")
		bool GetStopOnOverlap() const { return bStopOnOverlap; };

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Waypoint")
		float GetAcceptanceRadius() const { return AcceptanceRadius; };

	void CalculateSpline();

protected:
	UPROPERTY()
		TWeakObjectPtr<AWaypoint> WaypointCopiedFrom;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Waypoint|Components", meta = (AllowPrivateAccess = "true"))
		class USceneComponent* Scene;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Waypoint|Components", meta = (AllowPrivateAccess = "true"))
		class USphereComponent* OverlapSphere;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Waypoint|Components", meta = (AllowPrivateAccess = "true"))
		class UBillboardComponent* Sprite;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Waypoint|Components", meta = (AllowPrivateAccess = "true"))
		class USplineComponent* PathComponent;

	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Waypoint|Components", meta = (AllowPrivateAccess = "true"))
		class UArrowComponent* GuardFacingArrow;

	UPROPERTY(EditInstanceOnly, Category="Waypoint")
		TWeakObjectPtr<AWaypointLoop> OwningLoop;

protected:
	UFUNCTION(CallInEditor)
		void OnNavigationGenerationFinished(class ANavigationData* NavData);

	UFUNCTION(CallInEditor, Category = "Waypoint")
		void SelectNextWaypoint() const;

	UFUNCTION(CallInEditor, Category = "Waypoint")
		void CreateWaypointLoop();

	UPROPERTY(EditDefaultsOnly, Category = "Default")
		FNavAgentProperties NavProperties;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waypoint", meta = (ClampMin = "0.0", UIMin = "0.0"))
		float WaitTime;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waypoint")
		bool bOrientGuardToWaypoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waypoint")
		bool bStopOnOverlap;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Waypoint", meta = (ClampMin = "-1.0", UIMin = "-1.0"))
		float AcceptanceRadius;
};
