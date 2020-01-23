// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "WaypointsEditorExtensionModule.h"

#include "Waypoint.h"
#include "Engine.h"
#include "UnrealEdGlobals.h"
#include "Editor/UnrealEdEngine.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "LevelEditor.h"

#define LOCTEXT_NAMESPACE "FWaypointsEditorExtensionModule"

FDelegateHandle LevelViewportExtenderHandle;

class FWaypointsEditorExtensionModule_Impl : public IWaypointsEditorExtensionModule
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	static TSharedRef<FExtender> OnExtendLevelEditorActorContextMenu(const TSharedRef<FUICommandList> CommandList, const TArray<AActor*> SelectedActors);
	static void CreateWaypointsSelectionMenu(FMenuBuilder& MenuBuilder, const TArray<AWaypoint*> Waypoints);
};

void FWaypointsEditorExtensionModule_Impl::StartupModule()
{
	FLevelEditorModule& LevelEditorModule = FModuleManager::Get().LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	auto& MenuExtenders = LevelEditorModule.GetAllLevelViewportContextMenuExtenders();

	MenuExtenders.Add(FLevelEditorModule::FLevelViewportMenuExtender_SelectedActors::CreateStatic(&FWaypointsEditorExtensionModule_Impl::OnExtendLevelEditorActorContextMenu));
	LevelViewportExtenderHandle = MenuExtenders.Last().GetHandle();
}

void FWaypointsEditorExtensionModule_Impl::ShutdownModule()
{
	if (LevelViewportExtenderHandle.IsValid())
	{
		FLevelEditorModule* LevelEditorModule = FModuleManager::Get().GetModulePtr<FLevelEditorModule>("LevelEditor");
		if (LevelEditorModule)
		{
			typedef FLevelEditorModule::FLevelViewportMenuExtender_SelectedActors DelegateType;
			LevelEditorModule->GetAllLevelViewportContextMenuExtenders().RemoveAll([=](const DelegateType& In) { return In.GetHandle() == LevelViewportExtenderHandle; });
		}
	}
}

TSharedRef<FExtender> FWaypointsEditorExtensionModule_Impl::OnExtendLevelEditorActorContextMenu(const TSharedRef<FUICommandList> CommandList, const TArray<AActor*> SelectedActors)
{
	TSharedRef<FExtender> Extender(new FExtender());
	TArray<AWaypoint*> Waypoints;

	for (AActor* Actor : SelectedActors)
	{
		if (AWaypoint* Waypoint = Cast<AWaypoint>(Actor))
		{
			Waypoints.Push(Waypoint);
		}
	}

	// If the selection contains any WaypointActors
	if (Waypoints.Num() > 0)
	{
		Extender->AddMenuExtension(
			"SelectActorGeneral",
			EExtensionHook::After,
			CommandList,
			FMenuExtensionDelegate::CreateStatic(&FWaypointsEditorExtensionModule_Impl::CreateWaypointsSelectionMenu, Waypoints));
	}

	return Extender;
}

void FWaypointsEditorExtensionModule_Impl::CreateWaypointsSelectionMenu(FMenuBuilder& MenuBuilder, const TArray<AWaypoint*> Waypoints)
{
	MenuBuilder.AddMenuSeparator();
	MenuBuilder.AddMenuEntry(
		LOCTEXT("SelectWaypointLoop", "Select Waypoint Loop"),
		LOCTEXT("SelectWaypointsTooltip", ""),
		FSlateIcon(),
		FUIAction(FExecuteAction::CreateLambda([Waypoints]()
			{
				TArray<AWaypoint*> LoopWaypoints;
				for (auto Waypoint : Waypoints)
				{
					if (LoopWaypoints.Contains(Waypoint))
					{
						break;
					}

					LoopWaypoints.Append(Waypoint->GetLoop());
				}

				GEditor->GetSelectedActors()->BeginBatchSelectOperation();
				for (AWaypoint* Waypoint : LoopWaypoints)
				{
					GUnrealEd->SelectActor(Waypoint, true, false, false);
				}

				GEditor->GetSelectedActors()->EndBatchSelectOperation();
			}
		))
	);
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FWaypointsEditorExtensionModule_Impl, WaypointsEditorExtension)