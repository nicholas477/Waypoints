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
