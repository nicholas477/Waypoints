// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "WaypointsModule.h"

#include "BTTask_MoveToNextWaypoint.h"
#include "GameFramework/Actor.h"
#include "AISystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "VisualLogger/VisualLogger.h"
#include "AIController.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Tasks/AITask_MoveTo.h"

#include "Waypoint.h"

UBTTask_MoveToNextWaypoint::UBTTask_MoveToNextWaypoint(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	NodeName = "Move To Next Waypoint";
	bUseGameplayTasks = GET_AI_CONFIG_VAR(bEnableBTAITasks);
	bNotifyTick = !bUseGameplayTasks;
	bNotifyTaskFinished = true;

	AcceptanceRadius = 32.f;

	bReachTestIncludesGoalRadius = bReachTestIncludesAgentRadius = GET_AI_CONFIG_VAR(bFinishMoveOnGoalOverlap);
	//bAllowStrafe = GET_AI_CONFIG_VAR(bAllowStrafing);

	bWaitAtCheckpoint = true;

	// Accept only waypoints
	BlackboardKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_MoveToNextWaypoint, BlackboardKey), AWaypoint::StaticClass());
}

EBTNodeResult::Type UBTTask_MoveToNextWaypoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	EBTNodeResult::Type NodeResult = EBTNodeResult::InProgress;

	FBTMoveToNextWaypointTaskMemory* MyMemory = CastInstanceNodeMemory<FBTMoveToNextWaypointTaskMemory>(NodeMemory);
	MyMemory->PreviousGoalLocation = FAISystem::InvalidLocation;
	MyMemory->MoveRequestID = FAIRequestID::InvalidRequest;

	AAIController* MyController = OwnerComp.GetAIOwner();
	MyMemory->bWaitingForPath = bUseGameplayTasks ? false : MyController->ShouldPostponePathUpdates();
	if (!MyMemory->bWaitingForPath)
	{
		NodeResult = PerformMoveTask(OwnerComp, NodeMemory);
	}
	else
	{
		UE_VLOG(MyController, LogBehaviorTree, Log, TEXT("Pathfinding requests are freezed, waiting..."));
	}

	return NodeResult;
}

EBTNodeResult::Type UBTTask_MoveToNextWaypoint::PerformMoveTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	const UBlackboardComponent* MyBlackboard = OwnerComp.GetBlackboardComponent();
	FBTMoveToNextWaypointTaskMemory* MyMemory = CastInstanceNodeMemory<FBTMoveToNextWaypointTaskMemory>(NodeMemory);
	AAIController* MyController = OwnerComp.GetAIOwner();

	EBTNodeResult::Type NodeResult = EBTNodeResult::Failed;
	if (MyController && MyBlackboard)
	{
		FAIMoveRequest MoveReq;
		MoveReq.SetNavigationFilter(MyController->GetDefaultNavigationFilterClass());
		//MoveReq.SetCanStrafe(bAllowStrafe);
		MoveReq.SetUsePathfinding(true);
		MoveReq.SetStopOnOverlap(true);
		MoveReq.SetAllowPartialPath(true);

		if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Object::StaticClass())
		{
			UObject* KeyValue = MyBlackboard->GetValue<UBlackboardKeyType_Object>(BlackboardKey.GetSelectedKeyID());
			AWaypoint* TargetActor = Cast<AWaypoint>(KeyValue);
			if (TargetActor)
			{
				MoveReq.SetAcceptanceRadius(AcceptanceRadius);
				MoveReq.SetReachTestIncludesAgentRadius(bReachTestIncludesAgentRadius);
				MoveReq.SetReachTestIncludesGoalRadius(bReachTestIncludesGoalRadius);
				MoveReq.SetGoalActor(TargetActor);
			}
			else
			{
				UE_VLOG(MyController, LogBehaviorTree, Warning, TEXT("UBTTask_MoveToNextWaypoint::ExecuteTask tried to go to actor while BB %s entry was empty"), *BlackboardKey.SelectedKeyName.ToString());
			}
		}

		if (MoveReq.IsValid())
		{
			if (GET_AI_CONFIG_VAR(bEnableBTAITasks))
			{
				UAITask_MoveTo* MoveTask = MyMemory->Task.Get();
				const bool bReuseExistingTask = (MoveTask != nullptr);

				MoveTask = PrepareMoveTask(OwnerComp, MoveTask, MoveReq);
				if (MoveTask)
				{
					MyMemory->bObserverCanFinishTask = false;

					if (bReuseExistingTask)
					{
						if (MoveTask->IsActive())
						{
							UE_VLOG(MyController, LogBehaviorTree, Verbose, TEXT("\'%s\' reusing AITask %s"), *GetNodeName(), *MoveTask->GetName());
							MoveTask->ConditionalPerformMove();
						}
						else
						{
							UE_VLOG(MyController, LogBehaviorTree, Verbose, TEXT("\'%s\' reusing AITask %s, but task is not active - handing over move performing to task mechanics"), *GetNodeName(), *MoveTask->GetName());
						}
					}
					else
					{
						MyMemory->Task = MoveTask;
						UE_VLOG(MyController, LogBehaviorTree, Verbose, TEXT("\'%s\' task implementing move with task %s"), *GetNodeName(), *MoveTask->GetName());
						MoveTask->ReadyForActivation();
					}

					MyMemory->bObserverCanFinishTask = true;
					NodeResult = (MoveTask->GetState() != EGameplayTaskState::Finished) ? EBTNodeResult::InProgress :
						MoveTask->WasMoveSuccessful() ? EBTNodeResult::Succeeded :
						EBTNodeResult::Failed;
				}
			}
			else
			{
				FPathFollowingRequestResult RequestResult = MyController->MoveTo(MoveReq);
				if (RequestResult.Code == EPathFollowingRequestResult::RequestSuccessful)
				{
					MyMemory->MoveRequestID = RequestResult.MoveId;
					WaitForMessage(OwnerComp, UBrainComponent::AIMessage_MoveFinished, RequestResult.MoveId);
					WaitForMessage(OwnerComp, UBrainComponent::AIMessage_RepathFailed);

					NodeResult = EBTNodeResult::InProgress;
				}
				else if (RequestResult.Code == EPathFollowingRequestResult::AlreadyAtGoal)
				{
					NodeResult = EBTNodeResult::Succeeded;
				}
			}
		}
	}

	return NodeResult;
}

UAITask_MoveTo* UBTTask_MoveToNextWaypoint::PrepareMoveTask(UBehaviorTreeComponent& OwnerComp, UAITask_MoveTo* ExistingTask, FAIMoveRequest& MoveRequest)
{
	UAITask_MoveTo* MoveTask = ExistingTask ? ExistingTask : NewBTAITask<UAITask_MoveTo>(OwnerComp);
	if (MoveTask)
	{
		MoveTask->SetUp(MoveTask->GetAIController(), MoveRequest);
	}

	return MoveTask;
}

EBTNodeResult::Type UBTTask_MoveToNextWaypoint::AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	FBTMoveToNextWaypointTaskMemory* MyMemory = CastInstanceNodeMemory<FBTMoveToNextWaypointTaskMemory>(NodeMemory);
	if (!MyMemory->bWaitingForPath)
	{
		if (MyMemory->MoveRequestID.IsValid())
		{
			AAIController* MyController = OwnerComp.GetAIOwner();
			if (MyController && MyController->GetPathFollowingComponent())
			{
				MyController->GetPathFollowingComponent()->AbortMove(*this, FPathFollowingResultFlags::OwnerFinished, MyMemory->MoveRequestID);
			}
		}
		else
		{
			MyMemory->bObserverCanFinishTask = false;
			UAITask_MoveTo* MoveTask = MyMemory->Task.Get();
			if (MoveTask)
			{
				MoveTask->ExternalCancel();
			}
			else
			{
				UE_VLOG(&OwnerComp, LogBehaviorTree, Error, TEXT("Can't abort path following! bWaitingForPath:false, MoveRequestID:invalid, MoveTask:none!"));
			}
		}
	}

	return Super::AbortTask(OwnerComp, NodeMemory);
}

void UBTTask_MoveToNextWaypoint::OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult)
{
	FBTMoveToNextWaypointTaskMemory* MyMemory = CastInstanceNodeMemory<FBTMoveToNextWaypointTaskMemory>(NodeMemory);
	MyMemory->Task.Reset();

	// Set the blackboard value to the next waypoint
	if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Object::StaticClass())
	{
		UBlackboardComponent* MyBlackboard = OwnerComp.GetBlackboardComponent();
		UObject* KeyValue = MyBlackboard->GetValue<UBlackboardKeyType_Object>(BlackboardKey.GetSelectedKeyID());
		AWaypoint* TargetActor = Cast<AWaypoint>(KeyValue);
		if (TargetActor)
		{
			MyBlackboard->SetValueAsObject(BlackboardKey.SelectedKeyName, TargetActor->GetNextWaypoint());
		}
	}

	// Reset the AI's focus
	if (AAIController* MyController = OwnerComp.GetAIOwner())
	{
		MyController->ClearFocus(EAIFocusPriority::Gameplay);
	}

	Super::OnTaskFinished(OwnerComp, NodeMemory, TaskResult);
}

void UBTTask_MoveToNextWaypoint::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
	FBTMoveToNextWaypointTaskMemory* MyMemory = (FBTMoveToNextWaypointTaskMemory*)NodeMemory;

	if (MyMemory->bWaitingForPath && !OwnerComp.IsPaused())
	{
		AAIController* MyController = OwnerComp.GetAIOwner();
		if (MyController && !MyController->ShouldPostponePathUpdates())
		{
			UE_VLOG(MyController, LogBehaviorTree, Log, TEXT("Pathfinding requests are unlocked!"));
			MyMemory->bWaitingForPath = false;

			const EBTNodeResult::Type NodeResult = PerformMoveTask(OwnerComp, NodeMemory);
			if (NodeResult != EBTNodeResult::InProgress)
			{
				FinishLatentTask(OwnerComp, NodeResult);
			}
		}
	}

	// If remaining wait time >= 0.f then we should be waiting
	if (MyMemory->RemainingWaitTime > 0.f)
	{
		MyMemory->RemainingWaitTime -= DeltaSeconds;

		if (MyMemory->RemainingWaitTime <= 0.f)
		{
			FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
		}
	}
}

void UBTTask_MoveToNextWaypoint::OnMessage(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, FName Message, int32 SenderID, bool bSuccess)
{
	// AIMessage_RepathFailed means task has failed
	bSuccess &= (Message != UBrainComponent::AIMessage_RepathFailed);
	
	FBTMoveToNextWaypointTaskMemory* MyMemory = CastInstanceNodeMemory<FBTMoveToNextWaypointTaskMemory>(NodeMemory);
	const UBlackboardComponent* MyBlackboard = OwnerComp.GetBlackboardComponent();
	UObject* KeyValue = MyBlackboard->GetValue<UBlackboardKeyType_Object>(BlackboardKey.GetSelectedKeyID());
	AWaypoint* TargetActor = Cast<AWaypoint>(KeyValue);

	AAIController* MyController = OwnerComp.GetAIOwner();

	// We've finished moving to the waypoint, now time to wait
	if (bSuccess && TargetActor && bWaitAtCheckpoint && (TargetActor->GetWaitTime() > 0.f))
	{
		// Turn the actor towards the waypoint
		if (MyController && TargetActor->GetOrientGuardToWaypoint())
		{
			APawn* Pawn = MyController->GetPawn();
			const FVector PawnLocation = Pawn->GetActorLocation();
			const FVector DirectionVector = TargetActor->GetActorForwardVector();
			const FVector FocalPoint = PawnLocation + DirectionVector * 10000.0f;

			MyController->SetFocalPoint(FocalPoint, EAIFocusPriority::Gameplay);

		}

		MyMemory->RemainingWaitTime = TargetActor->GetWaitTime();
		return;
	}

	Super::OnMessage(OwnerComp, NodeMemory, Message, SenderID, bSuccess);
}

void UBTTask_MoveToNextWaypoint::OnGameplayTaskDeactivated(UGameplayTask& Task)
{
	// AI move task finished
	UAITask_MoveTo* MoveTask = Cast<UAITask_MoveTo>(&Task);
	if (MoveTask && MoveTask->GetAIController() && MoveTask->GetState() != EGameplayTaskState::Paused)
	{
		UBehaviorTreeComponent* BehaviorComp = GetBTComponentForTask(Task);
		if (BehaviorComp)
		{
			uint8* RawMemory = BehaviorComp->GetNodeMemory(this, BehaviorComp->FindInstanceContainingNode(this));
			FBTMoveToNextWaypointTaskMemory* MyMemory = CastInstanceNodeMemory<FBTMoveToNextWaypointTaskMemory>(RawMemory);

			if (MyMemory->bObserverCanFinishTask && (MoveTask == MyMemory->Task))
			{
				const bool bSuccess = MoveTask->WasMoveSuccessful();
				FinishLatentTask(*BehaviorComp, bSuccess ? EBTNodeResult::Succeeded : EBTNodeResult::Failed);
			}
		}
	}
}

FString UBTTask_MoveToNextWaypoint::GetStaticDescription() const
{
	FString KeyDesc("invalid");
	if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Object::StaticClass() ||
		BlackboardKey.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass())
	{
		KeyDesc = BlackboardKey.SelectedKeyName.ToString();
	}

	return FString::Printf(TEXT("%s: %s"), *Super::GetStaticDescription(), *KeyDesc);
}

void UBTTask_MoveToNextWaypoint::DescribeRuntimeValues(const UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const
{
	Super::DescribeRuntimeValues(OwnerComp, NodeMemory, Verbosity, Values);

	const UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();

	if (BlackboardComp)
	{
		const FString KeyValue = BlackboardComp->DescribeKeyValue(BlackboardKey.GetSelectedKeyID(), EBlackboardDescription::OnlyValue);

		FBTMoveToNextWaypointTaskMemory* MyMemory = (FBTMoveToNextWaypointTaskMemory*)NodeMemory;
		const bool bIsUsingTask = MyMemory->Task.IsValid();

		const FString ModeDesc =
			MyMemory->bWaitingForPath ? TEXT("(WAITING)") :
			bIsUsingTask ? TEXT("(task)") :
			TEXT("");

		Values.Add(FString::Printf(TEXT("move target: %s%s"), *KeyValue, *ModeDesc));
	}
}

uint16 UBTTask_MoveToNextWaypoint::GetInstanceMemorySize() const
{
	return sizeof(FBTMoveToNextWaypointTaskMemory);
}

#if WITH_EDITOR

FName UBTTask_MoveToNextWaypoint::GetNodeIconName() const
{
	return FName("BTEditor.Graph.BTNode.Task.MoveTo.Icon");
}

#endif	// WITH_EDITOR
