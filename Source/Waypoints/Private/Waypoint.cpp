// Copyright 2020 Nicholas Chalkley. All Rights Reserved.

#include "Waypoint.h"
#include "Components/SceneComponent.h"

#if WITH_EDITOR
#include "ObjectEditorUtils.h"
#include "UObject/ConstructorHelpers.h"
#if WITH_EDITORONLY_DATA
#include "Components/BillboardComponent.h"
#include "Components/SplineComponent.h"
#include "Components/ArrowComponent.h"
#endif // WITH_EDITORONLY_DATA
#endif // WITH_EDITOR

AWaypoint::AWaypoint(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	Scene = CreateDefaultSubobject<USceneComponent>(TEXT("SceneComponent"));
	SetRootComponent(Scene);

	AcceptanceRadius = 64.f;
	WaitTime = 0.f;

	bStopOnOverlap = true;
	bOrientGuardToWaypoint = false;

	PreviousWaypoint = this;
	NextWaypoint = this;

#if WITH_EDITORONLY_DATA
	// Structure to hold one-time initialization
	struct FConstructorStatics
	{
		// A helper class object we use to find target UTexture2D object in resource package
		ConstructorHelpers::FObjectFinderOptional<UTexture2D> NoteTextureObject;

		// Icon sprite category name
		FName ID_Notes;

		// Icon sprite display name
		FText NAME_Notes;

		FConstructorStatics()
			// Use helper class object to find the texture
			// "/Engine/EditorResources/S_Note" is resource path
			: NoteTextureObject(TEXT("/Engine/EditorResources/AI/S_NavLink"))
			, ID_Notes(TEXT("Notes"))
			, NAME_Notes(NSLOCTEXT("SpriteCategory", "Notes", "Notes"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

	Sprite = ObjectInitializer.CreateEditorOnlyDefaultSubobject<UBillboardComponent>(this, TEXT("Sprite"));
	if (Sprite)
	{

		Sprite->Sprite = ConstructorStatics.NoteTextureObject.Get();		// Get the sprite texture from helper class object
		Sprite->SpriteInfo.Category = ConstructorStatics.ID_Notes;		// Assign sprite category name
		Sprite->SpriteInfo.DisplayName = ConstructorStatics.NAME_Notes;	// Assign sprite display name
		Sprite->SetupAttachment(Scene);
	}

	PathComponent = ObjectInitializer.CreateEditorOnlyDefaultSubobject<USplineComponent>(this, TEXT("PathRenderComponent"));
	if (PathComponent)
	{
		PathComponent->SetupAttachment(Scene);
		PathComponent->bSelectable = false;
		PathComponent->bEditableWhenInherited = false;
		PathComponent->bAbsoluteLocation = true;
	}

	GuardFacingArrow = ObjectInitializer.CreateEditorOnlyDefaultSubobject<UArrowComponent>(this, TEXT("Guard Facing Arrow Component"));
	if (GuardFacingArrow)
	{
		GuardFacingArrow->SetupAttachment(Scene);
		GuardFacingArrow->bSelectable = false;
		GuardFacingArrow->bEditableWhenInherited = false;
		GuardFacingArrow->SetVisibility(false);
	}
#endif // WITH_EDITORONLY_DATA
}

TArray<AWaypoint*> AWaypoint::GetLoop()
{
	TArray<AWaypoint*> WaypointLoop = {this};

	AWaypoint* CurrentWaypoint = NextWaypoint.Get();
	while (CurrentWaypoint && CurrentWaypoint != this && !WaypointLoop.Contains(CurrentWaypoint))
	{
		WaypointLoop.Push(CurrentWaypoint);
		CurrentWaypoint = CurrentWaypoint->NextWaypoint.Get();
	}

	return WaypointLoop;
}

#if WITH_EDITOR
void AWaypoint::PostRegisterAllComponents()
{
	Super::PostRegisterAllComponents();

	const UWorld* World = GetWorld();
	if (World && World->WorldType == EWorldType::Editor)
	{
		UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
		if (NavSys)
		{
			NavSys->OnNavigationGenerationFinishedDelegate.RemoveAll(this);
			NavSys->OnNavigationGenerationFinishedDelegate.AddDynamic(this, &AWaypoint::OnNavigationGenerationFinished);
		}
	}
}

void AWaypoint::PreEditChange(UProperty* PropertyThatWillChange)
{
	static const FName NAME_NextWaypoint = GET_MEMBER_NAME_CHECKED(AWaypoint, NextWaypoint);

	if (PropertyThatWillChange && PropertyThatWillChange->GetFName() == NAME_NextWaypoint)
	{
		if (NextWaypoint.IsValid())
		{
			NextWaypoint->PreviousWaypoint = nullptr;
			NextWaypoint->CalculateSpline();
		}
	}

	Super::PreEditChange(PropertyThatWillChange);
}

void AWaypoint::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	static const FName NAME_NextWaypoint = GET_MEMBER_NAME_CHECKED(AWaypoint, NextWaypoint);
	static const FName NAME_bOrientGuardToWaypoint = GET_MEMBER_NAME_CHECKED(AWaypoint, bOrientGuardToWaypoint);

	Super::PostEditChangeProperty(PropertyChangedEvent);

	// This is called when the waypoint deletion is undo'ed
	if (PropertyChangedEvent.ChangeType == EPropertyChangeType::Unspecified && PreviousWaypoint.IsValid())
	{
		PreviousWaypoint->NextWaypoint = this;
		PreviousWaypoint->CalculateSpline();
	}

	if (PropertyChangedEvent.Property)
	{
		const FName ChangedPropName = PropertyChangedEvent.Property->GetFName();

		if (ChangedPropName == NAME_NextWaypoint)
		{
			CalculateSpline();
			if (NextWaypoint.IsValid())
			{
				NextWaypoint->PreviousWaypoint = this;
			}
		}

		if (ChangedPropName == NAME_bOrientGuardToWaypoint && GuardFacingArrow)
		{
			GuardFacingArrow->SetVisibility(bOrientGuardToWaypoint);
		}
	}
}

void AWaypoint::PostEditImport()
{
	Super::PostEditImport();

	if (!bHasBeenCopied && NextWaypoint.IsValid() && NextWaypoint->PreviousWaypoint != this)
	{
		// Look at the next link's previous link, set its next waypoint to this
		if (NextWaypoint->PreviousWaypoint.IsValid())
		{
			NextWaypoint->PreviousWaypoint->NextWaypoint = this;
			NextWaypoint->PreviousWaypoint->CalculateSpline();
		}
		PreviousWaypoint = NextWaypoint->PreviousWaypoint;
		NextWaypoint->PreviousWaypoint = this;

		CalculateSpline();
		if (PreviousWaypoint.IsValid())
		{
			PreviousWaypoint->CalculateSpline();
		}

		bHasBeenCopied = true;
	}
}

void AWaypoint::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);

	CalculateSpline();

	if (PreviousWaypoint.IsValid())
	{
		PreviousWaypoint->CalculateSpline();
	}
}

void AWaypoint::PostDuplicate(EDuplicateMode::Type DuplicateMode)
{
	Super::PostDuplicate(DuplicateMode);
}

void AWaypoint::CalculateSpline()
{
#if WITH_EDITORONLY_DATA
	if (NextWaypoint.IsValid() && NextWaypoint != this)
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
						for (int64 i = 0; i < SplinePoints.Num(); ++i)
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
#endif WITH_EDITORONLY_DATA
}

void AWaypoint::OnNavigationGenerationFinished(class ANavigationData* NavData)
{
	CalculateSpline();
}

void AWaypoint::Destroyed()
{
	RemoveThisWaypoint();

	Super::Destroyed();
}

void AWaypoint::RemoveThisWaypoint()
{
	// Go to the previous waypoint, and either remove this as the next waypoint or set its next waypoint to our next waypoint
	if (PreviousWaypoint.IsValid() && PreviousWaypoint->NextWaypoint == this)
	{
		//UE_LOG(LogTemp, Warning, TEXT("Setting previous waypoint's next waypoint"));
		PreviousWaypoint->NextWaypoint = NextWaypoint;
		PreviousWaypoint->CalculateSpline();
	}

	if (NextWaypoint.IsValid() && NextWaypoint->PreviousWaypoint == this)
	{
		//UE_LOG(LogTemp, Warning, TEXT("Setting next waypoint's previous waypoint"));
		NextWaypoint->PreviousWaypoint = PreviousWaypoint;
	}

}

#endif // WITH_EDITOR
