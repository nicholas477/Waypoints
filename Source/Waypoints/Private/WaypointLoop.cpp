// Fill out your copyright notice in the Description page of Project Settings.


#include "WaypointLoop.h"
#include "Waypoint.h"
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
	check(!Waypoints.Contains(TWeakObjectPtr<AWaypoint>(NewWaypoint)));

	Waypoints.Push(TWeakObjectPtr<AWaypoint>(NewWaypoint));

	// TODO: Update other waypoints when this happens
}

void AWaypointLoop::InsertWaypoint(AWaypoint* NewWaypoint, int32 Index)
{
	check(!Waypoints.Contains(TWeakObjectPtr<AWaypoint>(NewWaypoint)));

	Waypoints.Insert(NewWaypoint, Index);

	for (int i = 0; i < Waypoints.Num(); ++i)
	{
		UE_LOG(LogWaypoints, Warning, TEXT("Waypoints[%d]: %s"), i, *Waypoints[i]->GetName());
	}
}

void AWaypointLoop::RemoveWaypoint(const AWaypoint* Waypoint)
{
	// Remove the last matching element
	for (int32 i = Waypoints.Num() - 1; i >= 0; --i)
	{
		if (Waypoints[i].Get() == Waypoint)
		{
			Waypoints.RemoveAt(i);

			// Tell the previous waypoint to recalculate its spline
			if (Waypoints.Num() != 0)
			{
				int32 WrappedIndex = (i - 1) % Waypoints.Num();
				if (WrappedIndex < 0)
				{
					WrappedIndex += Waypoints.Num();
				}

				Waypoints[WrappedIndex]->CalculateSpline();
			}

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

int32 AWaypointLoop::FindWaypoint(const AWaypoint* Elem) const
{
	for (int32 i = 0; i < Waypoints.Num(); ++i)
	{
		if (Waypoints[i].Get() == Elem)
		{
			return i;
		}
	}

	return INDEX_NONE;
}