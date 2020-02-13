// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "WaypointLoop.generated.h"

class AWaypoint;

UCLASS()
class WAYPOINTS_API AWaypointLoop : public AActor
{
	GENERATED_UCLASS_BODY()

	UPROPERTY(EditInstanceOnly, Category="Waypoint Loop")
		TArray<AWaypoint*> Waypoints;
};
