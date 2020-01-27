// Copyright 2020 Nicholas Chalkley. All Rights Reserved.


#include "BTTask_MoveToNextWaypoint.h"

UBTTask_MoveToNextWaypoint::UBTTask_MoveToNextWaypoint(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	NodeName = "Move To Next Waypoint";
	//bUseGameplayTasks = GET_AI_CONFIG_VAR(bEnableBTAITasks);
	//bNotifyTick = true; // !bUseGameplayTasks;
	//bNotifyTaskFinished = true;

	//AcceptableRadius = GET_AI_CONFIG_VAR(AcceptanceRadius);
	//bReachTestIncludesGoalRadius = bReachTestIncludesAgentRadius = bStopOnOverlap = GET_AI_CONFIG_VAR(bFinishMoveOnGoalOverlap);
	//bAllowStrafe = GET_AI_CONFIG_VAR(bAllowStrafing);
	//bAllowPartialPath = GET_AI_CONFIG_VAR(bAcceptPartialPaths);
	//bTrackMovingGoal = true;
	//bProjectGoalLocation = true;
	//bUsePathfinding = true;

	//bStopOnOverlapNeedsUpdate = true;

	//ObservedBlackboardValueTolerance = AcceptableRadius * 0.95f;

	// accept only actors and vectors
	//BlackboardKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_MoveToNextWaypoint, BlackboardKey), AActor::StaticClass());
	//BlackboardKey.AddVectorFilter(this, GET_MEMBER_NAME_CHECKED(UBTTask_MoveToNextWaypoint, BlackboardKey));
}

EBTNodeResult::Type UBTTask_MoveToNextWaypoint::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) 
{
	EBTNodeResult::Type NodeResult = EBTNodeResult::InProgress;
	UE_LOG(LogTemp, Warning, TEXT("asdf"));


	return NodeResult;
}