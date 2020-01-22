// Copyright 2020 Nicholas Chalkley. All Rights Reserved

#pragma once

#include "CoreMinimal.h"
#include "NavigationSystem.h"
#include "GameFramework/Actor.h"
#include "Waypoint.generated.h"

UCLASS()
class WAYPOINTS_API AWaypoint : public AActor
{
	GENERATED_UCLASS_BODY()

#if WITH_EDITOR
	virtual void PostRegisterAllComponents() override;
	virtual void PreEditChange(UProperty* PropertyThatWillChange) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void PostEditMove(bool bFinished) override;
	virtual void PostDuplicate(EDuplicateMode::Type DuplicateMode) override;
	virtual void PostEditImport() override;

	virtual bool CanDeleteSelectedActor(FText& OutReason) const override { return true; };
	virtual void Destroyed() override;

protected:
	bool bHasBeenCopied = false;

	virtual void RemoveThisWaypoint();

#if WITH_EDITORONLY_DATA
	UPROPERTY()
		class UBillboardComponent* Sprite;

	UPROPERTY()
		class USplineComponent* PathComponent;
		
	UPROPERTY()
		class UArrowComponent* GuardFacingArrow;
#endif // WITH_EDITORONLY_DATA

	void CalculateSpline();

	UFUNCTION(CallInEditor)
		void OnNavigationGenerationFinished(class ANavigationData* NavData);

#endif // WITH_EDITOR

protected:
	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Default")
		TWeakObjectPtr<AWaypoint> NextWaypoint;

	UPROPERTY()
		TWeakObjectPtr<AWaypoint> PreviousWaypoint;

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
