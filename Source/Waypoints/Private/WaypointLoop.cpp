// Fill out your copyright notice in the Description page of Project Settings.


#include "WaypointLoop.h"
#include "Components/SceneComponent.h"

// Sets default values
AWaypointLoop::AWaypointLoop(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Scene = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	SetRootComponent(Scene);
}

void AWaypointLoop::AddWaypoint(AWaypoint* NewWaypoint)
{
	check(!Waypoints.Contains(NewWaypoint));

	Waypoints.Push(NewWaypoint);

	// TODO: Update other waypoints when this happens
}

void AWaypointLoop::RemoveWaypoint(const AWaypoint* Waypoint)
{
	// Remove the last matching element
	for (int32 i = Waypoints.Num() - 1; i >= 0; --i)
	{
		if (Waypoints[i] == Waypoint)
		{
			Waypoints.RemoveAt(i);
			break;
		}
	}

	// Destroy this waypoint loop if there's no waypoints
	if (Waypoints.Num() == 0)
	{
		Destroy();
	}

	// TODO: Update other waypoints when this happens
}
