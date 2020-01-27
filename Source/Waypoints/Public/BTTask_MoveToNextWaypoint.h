// Copyright 2020 Nicholas Chalkley. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Tasks/BTTask_BlackboardBase.h"
#include "BTTask_MoveToNextWaypoint.generated.h"

/**
 * 
 */
UCLASS()
class WAYPOINTS_API UBTTask_MoveToNextWaypoint : public UBTTask_BlackboardBase
{
	GENERATED_UCLASS_BODY()

	virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
	virtual EBTNodeResult::Type AbortTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override { return EBTNodeResult::Failed; };
	virtual void OnTaskFinished(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTNodeResult::Type TaskResult) override {};
	virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override {};
	virtual uint16 GetInstanceMemorySize() const override { return 0; };
	virtual void PostLoad() override { Super::PostLoad();  };

	virtual void OnGameplayTaskDeactivated(UGameplayTask& Task) override {};
	virtual void OnMessage(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, FName Message, int32 RequestID, bool bSuccess) override {};
	EBlackboardNotificationResult OnBlackboardValueChange(const UBlackboardComponent& Blackboard, FBlackboard::FKey ChangedKeyID) { return EBlackboardNotificationResult::ContinueObserving; };

	virtual void DescribeRuntimeValues(const UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, EBTDescriptionVerbosity::Type Verbosity, TArray<FString>& Values) const override {};
	virtual FString GetStaticDescription() const override { return ""; };

#if WITH_EDITOR
	virtual FName GetNodeIconName() const override { return FName("BTEditor.Graph.BTNode.Task.MoveTo.Icon");; };
	virtual void OnNodeCreated() override {};
#endif // WITH_EDITOR
};
