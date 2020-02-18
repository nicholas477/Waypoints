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

	AcceptanceRadius = 64.f;
	WaitTime = 0.f;

	bHasBeenCopied = false;
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

void AWaypoint::PostInitProperties()
{
	Super::PostInitProperties();

	UE_LOG(LogTemp, Warning, TEXT("AWaypoint::PostInitProperties()"));

	UE_LOG(LogTemp, Warning, TEXT(""), this->AcceptanceRadius);

}

TArray<AWaypoint*> AWaypoint::GetLoop() const
{
	//AWaypointLoop* OwningLoop = Cast<AWaypointLoop>(GetAttachParentActor());
	if (OwningLoop)
	{
		return OwningLoop->Waypoints;
	}

	return {};
}

AWaypoint* AWaypoint::GetNextWaypoint() const
{
	//AWaypointLoop* OwningLoop = Cast<AWaypointLoop>(GetAttachParentActor());
	if (OwningLoop)
	{
		const TArray<AWaypoint*>& WaypointLoop = OwningLoop->Waypoints;
		if (auto Index = WaypointLoop.Find(const_cast<AWaypoint*>(this)) != INDEX_NONE)
		{
			return WaypointLoop[(Index + 1) % WaypointLoop.Num()];
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

	UE_LOG(LogTemp, Warning, TEXT("AWaypoint::PostRegisterAllComponents"));
#endif // WITH_EDITOR
}

void AWaypoint::PreEditChange(UProperty* PropertyThatWillChange)
{
//#if WITH_EDITOR
//	static const FName NAME_NextWaypoint = GET_MEMBER_NAME_CHECKED(AWaypoint, NextWaypoint);
//
//	if (PropertyThatWillChange && PropertyThatWillChange->GetFName() == NAME_NextWaypoint)
//	{
//		if (NextWaypoint.IsValid())
//		{
//			NextWaypoint->PreviousWaypoint = nullptr;
//			NextWaypoint->CalculateSpline();
//		}
//	}
//#endif // WITH_EDITOR

	Super::PreEditChange(PropertyThatWillChange);
}

void AWaypoint::PostActorCreated()
{
	Super::PostActorCreated();

	UE_LOG(LogTemp, Warning, TEXT("AWaypoint::PostActorCreated"));

	///UE_LOG(LogTemp, Warning, TEXT("Needs load? %s"), HasAnyFlags(EObjectFlags::RF_ArchetypeObject) ? TEXT("Yes") : TEXT("No"));

	//UWorld* World = GetWorld();
	//if (World && IsCorrectWorldType(World) && !HasAnyFlags(EObjectFlags::RF_ClassDefaultObject | EObjectFlags::RF_ArchetypeObject | EObjectFlags::RF_DuplicateTransient))
	//{
	//	//AWaypointLoop* OwningLoop = Cast<AWaypointLoop>(GetAttachParentActor());
	//	if (!OwningLoop)
	//	{
	//		UE_LOG(LogTemp, Warning, TEXT("Spawning new waypoint loop!!!!!"));
	//		FActorSpawnParameters Params;
	//		Params.bAllowDuringConstructionScript = true;
	//		OwningLoop = World->SpawnActor<AWaypointLoop>(AWaypointLoop::StaticClass(), Params);

	//		SetOwner(OwningLoop);
	//		AttachToActor(OwningLoop, FAttachmentTransformRules::KeepWorldTransform);
	//	}
	//}
}

void AWaypoint::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	UE_LOG(LogTemp, Warning, TEXT("AWaypoint::PostInitializeComponents"));
}

void AWaypoint::RegisterAllComponents()
{
	Super::RegisterAllComponents();

	UE_LOG(LogTemp, Warning, TEXT("AWaypoint::RegisterAllComponents"));
}

void AWaypoint::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	UE_LOG(LogTemp, Warning, TEXT("AWaypoint::OnConstruction"));
	//UE_LOG(LogTemp, Warning, TEXT("AWaypoint::OnConstruction: bActorIsBeingConstructed? %s"), bActorIsBeingConstructed ? TEXT("true") : TEXT("false"));
}

void AWaypoint::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

#if WITH_EDITOR
	//static const FName NAME_NextWaypoint = GET_MEMBER_NAME_CHECKED(AWaypoint, NextWaypoint);
	static const FName NAME_bOrientGuardToWaypoint = GET_MEMBER_NAME_CHECKED(AWaypoint, bOrientGuardToWaypoint);
	static const FName NAME_OwningLoop = GET_MEMBER_NAME_CHECKED(AWaypoint, OwningLoop);

	// This is called when the waypoint deletion is undo'ed
	//if (PropertyChangedEvent.ChangeType == EPropertyChangeType::Unspecified && PreviousWaypoint.IsValid())
	//{
	//	PreviousWaypoint->NextWaypoint = this;
	//	PreviousWaypoint->CalculateSpline();
	//}

	if (PropertyChangedEvent.Property)
	{
		const FName ChangedPropName = PropertyChangedEvent.Property->GetFName();

		//if (ChangedPropName == NAME_NextWaypoint)
		//{
		//	CalculateSpline();
		//	if (NextWaypoint.IsValid())
		//	{
		//		NextWaypoint->PreviousWaypoint = this;
		//	}
		//}

		if (ChangedPropName == NAME_bOrientGuardToWaypoint && GuardFacingArrow)
		{
			GuardFacingArrow->SetVisibility(bOrientGuardToWaypoint);
		}

		//if (ChangedPropName == NAME_OwningLoop)
		//{
		//	UWorld* World = GetWorld();
		//	if (World && IsCorrectWorldType(World) && !HasAnyFlags(EObjectFlags::RF_ClassDefaultObject | EObjectFlags::RF_ArchetypeObject | EObjectFlags::RF_DuplicateTransient))
		//	{
		//		if (!OwningLoop)
		//		{
		//			UE_LOG(LogTemp, Warning, TEXT("Spawning new waypoint loop!!!!!"));
		//			FActorSpawnParameters Params;
		//			Params.bAllowDuringConstructionScript = true;
		//			OwningLoop = World->SpawnActor<AWaypointLoop>(AWaypointLoop::StaticClass(), Params);

		//			SetOwner(OwningLoop);
		//			AttachToActor(OwningLoop, FAttachmentTransformRules::KeepWorldTransform);
		//		}
		//	}
		//}
	}

	UE_LOG(LogTemp, Warning, TEXT("AWaypoint::PostEditChangeProperty"));
#endif // WITH_EDITOR
}

void AWaypoint::PostEditImport()
{
	Super::PostEditImport();

#if WITH_EDITOR
	//if (!bHasBeenCopied && NextWaypoint.IsValid() && NextWaypoint->PreviousWaypoint != this)
	//{
	//	// Look at the next link's previous link, set its next waypoint to this
	//	if (NextWaypoint->PreviousWaypoint.IsValid())
	//	{
	//		NextWaypoint->PreviousWaypoint->NextWaypoint = this;
	//		NextWaypoint->PreviousWaypoint->CalculateSpline();
	//	}
	//	PreviousWaypoint = NextWaypoint->PreviousWaypoint;
	//	NextWaypoint->PreviousWaypoint = this;

	//	CalculateSpline();
	//	if (PreviousWaypoint.IsValid())
	//	{
	//		PreviousWaypoint->CalculateSpline();
	//	}

	//	bHasBeenCopied = true;
	//}

	UE_LOG(LogTemp, Warning, TEXT("AWaypoint::PostEditImport"));
#endif // WITH_EDITOR
}

void AWaypoint::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);

#if WITH_EDITOR
	CalculateSpline();

	//if (PreviousWaypoint.IsValid())
	//{
	//	PreviousWaypoint->CalculateSpline();
	//}
#endif // WITH_EDITOR
}

void AWaypoint::PostDuplicate(EDuplicateMode::Type DuplicateMode)
{
	Super::PostDuplicate(DuplicateMode);

	UWorld* World = GetWorld();
	if (World && IsCorrectWorldType(World) && !HasAnyFlags(EObjectFlags::RF_ClassDefaultObject | EObjectFlags::RF_ArchetypeObject | EObjectFlags::RF_DuplicateTransient))
	{
		if (!OwningLoop)
		{
			UE_LOG(LogTemp, Warning, TEXT("Spawning new waypoint loop!!!!!"));
			FActorSpawnParameters Params;
			Params.bAllowDuringConstructionScript = true;
			OwningLoop = World->SpawnActor<AWaypointLoop>(AWaypointLoop::StaticClass(), Params);

			SetOwner(OwningLoop);
			AttachToActor(OwningLoop, FAttachmentTransformRules::KeepWorldTransform);
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("AWaypoint::PostDuplicate"));
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

void AWaypoint::BeginDestroy()
{
	RemoveThisWaypoint();

	UE_LOG(LogTemp, Warning, TEXT("AWaypoint::BeginDestroy()"));

	Super::BeginDestroy();
}

void AWaypoint::Destroyed()
{
	RemoveThisWaypoint();

	UE_LOG(LogTemp, Warning, TEXT("AWaypoint::Destroyed()"));

	Super::Destroyed();
}

void AWaypoint::RemoveThisWaypoint()
{
#if WITH_EDITOR
	//AWaypointLoop* OwningLoop = Cast<AWaypointLoop>(GetAttachParentActor());
	if (OwningLoop)
	{
		OwningLoop->Destroy();
		OwningLoop = nullptr;
	}
#endif // WITH_EDITOR
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
