// Copyright 2020 Nicholas Chalkley. All Rights Reserved.

#include "Waypoint.h"
#include "Components/SceneComponent.h"
#include "WaypointLoop.h"

#if WITH_EDITOR
#include "ObjectEditorUtils.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/BillboardComponent.h"
#include "Components/SplineComponent.h"
#include "Components/ArrowComponent.h"
#include "Components/SphereComponent.h"

#include "Editor/UnrealEdEngine.h"
#include "Engine/Selection.h"
#endif // WITH_EDITOR

static bool IsCorrectWorldType(const UWorld* World)
{
	return World->WorldType == EWorldType::Editor;
	/*|| World->WorldType == EWorldType::Game
	|| World->WorldType == EWorldType::PIE;*/
}

AWaypoint::AWaypoint(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Scene = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	SetRootComponent(Scene);

	AcceptanceRadius = 128.f;
	WaitTime = 0.f;

//	bHasBeenCopied = false;
	bStopOnOverlap = true;
	bOrientGuardToWaypoint = false;

	bRunConstructionScriptOnDrag = false;

	//PreviousWaypoint = this;
	//NextWaypoint = this;

#if WITH_EDITOR
	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinderOptional<UTexture2D> WaypointIcon;

		FName ID_WaypointIcon;

		FText NAME_WaypointIcon;

		FConstructorStatics()
			: WaypointIcon(TEXT("/Waypoints/EditorResources/S_Waypoint"))
			, ID_WaypointIcon(TEXT("WaypointIcon"))
			, NAME_WaypointIcon(NSLOCTEXT("SpriteCategory", "Waypoints", "WaypointIcon"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

	Sprite = ObjectInitializer.CreateEditorOnlyDefaultSubobject<UBillboardComponent>(this, TEXT("Icon"));
	if (Sprite)
	{

		Sprite->Sprite = ConstructorStatics.WaypointIcon.Get();
		Sprite->SpriteInfo.Category = ConstructorStatics.ID_WaypointIcon;
		Sprite->SpriteInfo.DisplayName = ConstructorStatics.NAME_WaypointIcon;
		Sprite->SetupAttachment(Scene);
	}

	PathComponent = ObjectInitializer.CreateEditorOnlyDefaultSubobject<USplineComponent>(this, TEXT("PathRenderComponent"));
	if (PathComponent)
	{
		PathComponent->SetupAttachment(Scene);
		PathComponent->bSelectable = false;
		PathComponent->bEditableWhenInherited = false;
		PathComponent->SetUsingAbsoluteLocation(true);
		PathComponent->SetUsingAbsoluteRotation(true);
		PathComponent->SetUsingAbsoluteScale(true);
	}

	OverlapSphere = ObjectInitializer.CreateEditorOnlyDefaultSubobject<USphereComponent>(this, TEXT("Overlap Sphere Visualization Component"));
	if (OverlapSphere)
	{
		OverlapSphere->SetupAttachment(Scene);
		OverlapSphere->bSelectable = false;
		OverlapSphere->bEditableWhenInherited = false;
		OverlapSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		OverlapSphere->SetCollisionResponseToAllChannels(ECollisionResponse::ECR_Ignore);
		OverlapSphere->SetCollisionResponseToChannel(ECollisionChannel::ECC_Pawn, ECR_Overlap);
		OverlapSphere->SetSphereRadius(AcceptanceRadius);
	}

	GuardFacingArrow = ObjectInitializer.CreateEditorOnlyDefaultSubobject<UArrowComponent>(this, TEXT("Guard Facing Arrow Component"));
	if (GuardFacingArrow)
	{
		GuardFacingArrow->SetupAttachment(Scene);
		GuardFacingArrow->bSelectable = false;
		GuardFacingArrow->bEditableWhenInherited = false;
		GuardFacingArrow->SetVisibility(false);
		GuardFacingArrow->ArrowColor = FColor(255, 255, 255, 255);
	}
#endif // WITH_EDITOR
}

//void AWaypoint::BeginPlay()
//{
//	Super::BeginPlay();
//
//	UE_LOG(LogTemp, Warning, TEXT("AWaypoint::BeginPlay()"));
//	
//	//if (IsCorrectWorldType(World) && !Cast<const AWaypointLoop>(GetOwner()))
//	//{
//	//	FActorSpawnParameters Params;
//	//	Params.bAllowDuringConstructionScript = true;
//	//	auto* NewWaypointLoop = World->SpawnActor<AWaypointLoop>(AWaypointLoop::StaticClass(), Params);
//
//	//	//SetOwner(NewWaypointLoop);
//	//	//Rename(nullptr, NewWaypointLoop);
//	//	AttachToActor(NewWaypointLoop, FAttachmentTransformRules::KeepWorldTransform);
//	//}
//}

//void AWaypoint::PostInitProperties()
//{
//	Super::PostInitProperties();
//
//	UE_LOG(LogTemp, Warning, TEXT("AWaypoint::PostInitProperties()"));
//
//	UE_LOG(LogTemp, Warning, TEXT(""), this->AcceptanceRadius);
//
//}

TArray<TWeakObjectPtr<AWaypoint>> AWaypoint::GetLoop() const
{
	if (OwningLoop.IsValid())
	{
		return OwningLoop->Waypoints;
	}

	return {};
}

AWaypoint* AWaypoint::GetNextWaypoint() const
{
	if (OwningLoop.IsValid())
	{
		const TArray<TWeakObjectPtr<AWaypoint>>& WaypointLoop = OwningLoop->Waypoints;
		auto Index = OwningLoop->FindWaypoint(this);
		if (Index != INDEX_NONE)
		{
			return WaypointLoop[(Index + 1) % WaypointLoop.Num()].Get();
		}
	}

	return nullptr;
}

AWaypoint* AWaypoint::GetPreviousWaypoint() const
{
	if (OwningLoop.IsValid())
	{
		const TArray<TWeakObjectPtr<AWaypoint>>& WaypointLoop = OwningLoop->Waypoints;
		auto Index = OwningLoop->FindWaypoint(this);
		if (Index != INDEX_NONE)
		{
			int32 WrappedIndex = (Index - 1) % WaypointLoop.Num();
			if (WrappedIndex < 0)
			{
				WrappedIndex += WaypointLoop.Num();
			}

			return WaypointLoop[WrappedIndex].Get();
		}
	}

	return nullptr;
}

void AWaypoint::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();
	 
#if WITH_EDITOR
	UWorld* World = GetWorld();
	if (World && World->WorldType == EWorldType::Editor)
	{
		UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
		if (NavSys)
		{
			NavSys->OnNavigationGenerationFinishedDelegate.RemoveAll(this);
			NavSys->OnNavigationGenerationFinishedDelegate.AddDynamic(this, &AWaypoint::OnNavigationGenerationFinished);
		}
	}

	//UE_LOG(LogTemp, Warning, TEXT("AWaypoint::PostRegisterAllComponents"));
#endif // WITH_EDITOR
}

void AWaypoint::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

#if WITH_EDITOR
	//static const FName NAME_NextWaypoint = GET_MEMBER_NAME_CHECKED(AWaypoint, NextWaypoint);
	static const FName NAME_bOrientGuardToWaypoint = GET_MEMBER_NAME_CHECKED(AWaypoint, bOrientGuardToWaypoint);
	static const FName NAME_AcceptanceRadius = GET_MEMBER_NAME_CHECKED(AWaypoint, AcceptanceRadius);

	if (PropertyChangedEvent.Property)
	{
		const FName ChangedPropName = PropertyChangedEvent.Property->GetFName();

		if (ChangedPropName == NAME_bOrientGuardToWaypoint && GuardFacingArrow)
		{
			GuardFacingArrow->SetVisibility(bOrientGuardToWaypoint);
		}

		if (ChangedPropName == NAME_AcceptanceRadius)
		{
			OverlapSphere->SetSphereRadius(AcceptanceRadius);
		} 
	}

	//UE_LOG(LogWaypoints, Warning, TEXT("AWaypoint::PostEditChangeProperty"));
#endif // WITH_EDITOR
}

void AWaypoint::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);

#if WITH_EDITOR
	CalculateSpline();

	AWaypoint* PreviousWaypoint = GetPreviousWaypoint();
	if (PreviousWaypoint && PreviousWaypoint != this)
	{
		PreviousWaypoint->CalculateSpline();
	}
#endif // WITH_EDITOR
}

void AWaypoint::PostDuplicate(EDuplicateMode::Type DuplicateMode)
{
	Super::PostDuplicate(DuplicateMode);

#if WITH_EDITOR

	if (DuplicateMode != EDuplicateMode::Normal)
		return;

	if (OwningLoop.IsValid() && WaypointCopiedFrom.IsValid())
	{
		auto Index = OwningLoop->FindWaypoint(WaypointCopiedFrom.Get());
		if (Index != INDEX_NONE)
		{
			OwningLoop->InsertWaypoint(this, Index + 1);
			WaypointCopiedFrom = this;
		}
	}

	//UE_LOG(LogTemp, Warning, TEXT("AWaypoint::PostDuplicate"));

#endif
}

void AWaypoint::CalculateSpline()
{
#if WITH_EDITOR
	AWaypoint* NextWaypoint = GetNextWaypoint();
	if (NextWaypoint && NextWaypoint != this)
	{
		PathComponent->SetVisibility(true);

		UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
		if (NavSys)
		{
			FPathFindingQuery NavParams;
			NavParams.StartLocation = GetActorLocation();
			NavParams.EndLocation = NextWaypoint->GetActorLocation();

			FNavPathQueryDelegate Delegate;
			Delegate.BindLambda([this](uint32 aPathId, ENavigationQueryResult::Type, FNavPathSharedPtr NavPointer)
				{
					// Since this lambda is async it can be called after the object was deleted
					if (!IsValid(this) || !IsValid(this->PathComponent))
						return;

					FOccluderVertexArray SplinePoints;
					for (const auto NavPoint : NavPointer->GetPathPoints())
					{
						SplinePoints.Push(NavPoint.Location + FVector(0.f, 0.f, 128.f));
					}

					PathComponent->SetSplineWorldPoints(SplinePoints);
					if (SplinePoints.Num() > 1)
					{
						for (int32 i = 0; i < SplinePoints.Num(); ++i)
						{
							PathComponent->SetTangentsAtSplinePoint(i, FVector(0.f, 0.f, 0.f), FVector(0.f, 0.f, 0.f), ESplineCoordinateSpace::World);
						}
					}
				});
			NavSys->FindPathAsync(NavProperties, NavParams, Delegate);
		}
	}
	else
	{
		PathComponent->SetSplineWorldPoints(FOccluderVertexArray());
	}
#endif // WITH_EDITOR
}

void AWaypoint::OnNavigationGenerationFinished(class ANavigationData* NavData)
{
	CalculateSpline();
}

void AWaypoint::Destroyed()
{
	if (OwningLoop.IsValid())
	{
		OwningLoop->RemoveWaypoint(this);
		OwningLoop = nullptr;
	}

	//UE_LOG(LogWaypoints, Warning, TEXT("AWaypoint::Destroyed()"));

	Super::Destroyed();
}

void AWaypoint::SelectNextWaypoint() const
{
#if WITH_EDITOR
	USelection* Selection = GEditor->GetSelectedActors();

	if (AWaypoint* NextWaypoint = GetNextWaypoint())
	{
		Selection->DeselectAll();
		Selection->Select(NextWaypoint);
	}
#endif // WITH_EDITOR
}

void AWaypoint::CreateWaypointLoop()
{
#if WITH_EDITOR
	if (UWorld* World = GetWorld())
	{
		//UE_LOG(LogWaypoints, Warning, TEXT("Spawning new waypoint loop!!!!!"));
		FActorSpawnParameters Params;
		Params.bAllowDuringConstructionScript = true;
		OwningLoop = World->SpawnActor<AWaypointLoop>(AWaypointLoop::StaticClass(), Params);

		AttachToActor(OwningLoop.Get(), FAttachmentTransformRules::KeepWorldTransform);

		OwningLoop->AddWaypoint(this);
		WaypointCopiedFrom = this;
	}
#endif // WITH_EDITOR
}
