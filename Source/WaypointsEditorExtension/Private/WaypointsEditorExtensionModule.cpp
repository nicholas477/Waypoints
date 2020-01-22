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
				GEditor->GetSelectedActors()->BeginBatchSelectOperation();
				// Select all light actors.
				for (AWaypoint* Waypoint : TActorRange<AWaypoint>(Waypoints[0]->GetWorld()))
				{
					GUnrealEd->SelectActor(Waypoint, true, false, false);
				}

				GEditor->GetSelectedActors()->EndBatchSelectOperation();
			}
		))
	);
	//{
	//	MenuBuilder.AddSubMenu(
	//		LOCTEXT("ScriptedActorActions", "Scripted Actions"),
	//		LOCTEXT("ScriptedActorActionsTooltip", "Scripted actions available for the selected actors"),
	//		FNewMenuDelegate::CreateLambda([FunctionsToList](FMenuBuilder& InMenuBuilder)
	//			{
	//				for (const FFunctionAndUtil& FunctionAndUtil : FunctionsToList)
	//				{
	//					const FText TooltipText = FText::Format(LOCTEXT("AssetUtilTooltipFormat", "{0}\n(Shift-click to edit script)"), FunctionAndUtil.Function->GetToolTipText());

	//					InMenuBuilder.AddMenuEntry(
	//						FunctionAndUtil.Function->GetDisplayNameText(),
	//						TooltipText,
	//						FSlateIcon("EditorStyle", "GraphEditor.Event_16x"),
	//						FExecuteAction::CreateLambda([FunctionAndUtil]
	//							{
	//								if (FSlateApplication::Get().GetModifierKeys().IsShiftDown())
	//								{
	//									// Edit the script if we have shift held down
	//									if (UBlueprint* Blueprint = Cast<UBlueprint>(Cast<UObject>(FunctionAndUtil.Util)->GetClass()->ClassGeneratedBy))
	//									{
	//										if (IAssetEditorInstance* AssetEditor = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->FindEditorForAsset(Blueprint, true))
	//										{
	//											check(AssetEditor->GetEditorName() == TEXT("BlueprintEditor"));
	//											IBlueprintEditor* BlueprintEditor = static_cast<IBlueprintEditor*>(AssetEditor);
	//											BlueprintEditor->JumpToHyperlink(FunctionAndUtil.Function, false);
	//										}
	//										else
	//										{
	//											FBlueprintEditorModule& BlueprintEditorModule = FModuleManager::LoadModuleChecked<FBlueprintEditorModule>("Kismet");
	//											TSharedRef<IBlueprintEditor> BlueprintEditor = BlueprintEditorModule.CreateBlueprintEditor(EToolkitMode::Standalone, TSharedPtr<IToolkitHost>(), Blueprint, false);
	//											BlueprintEditor->JumpToHyperlink(FunctionAndUtil.Function, false);
	//										}
	//									}
	//								}
	//								else
	//								{
	//									// We dont run this on the CDO, as bad things could occur!
	//									UObject* TempObject = NewObject<UObject>(GetTransientPackage(), Cast<UObject>(FunctionAndUtil.Util)->GetClass());
	//									TempObject->AddToRoot(); // Some Blutility actions might run GC so the TempObject needs to be rooted to avoid getting destroyed

	//									if (FunctionAndUtil.Function->NumParms > 0)
	//									{
	//										// Create a parameter struct and fill in defaults
	//										TSharedRef<FStructOnScope> FuncParams = MakeShared<FStructOnScope>(FunctionAndUtil.Function);
	//										for (TFieldIterator<UProperty> It(FunctionAndUtil.Function); It&& It->HasAnyPropertyFlags(CPF_Parm); ++It)
	//										{
	//											FString Defaults;
	//											if (UEdGraphSchema_K2::FindFunctionParameterDefaultValue(FunctionAndUtil.Function, *It, Defaults))
	//											{
	//												It->ImportText(*Defaults, It->ContainerPtrToValuePtr<uint8>(FuncParams->GetStructMemory()), PPF_None, nullptr);
	//											}
	//										}

	//										// pop up a dialog to input params to the function
	//										TSharedRef<SWindow> Window = SNew(SWindow)
	//											.Title(FunctionAndUtil.Function->GetDisplayNameText())
	//											.ClientSize(FVector2D(400, 200))
	//											.SupportsMinimize(false)
	//											.SupportsMaximize(false);

	//										TSharedPtr<SFunctionParamDialog> Dialog;
	//										Window->SetContent(
	//											SAssignNew(Dialog, SFunctionParamDialog, Window, FuncParams)
	//											.OkButtonText(LOCTEXT("OKButton", "OK"))
	//											.OkButtonTooltipText(FunctionAndUtil.Function->GetToolTipText()));

	//										GEditor->EditorAddModalWindow(Window);

	//										if (Dialog->bOKPressed)
	//										{
	//											FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "BlutilityAction", "Blutility Action"));
	//											FEditorScriptExecutionGuard ScriptGuard;
	//											TempObject->ProcessEvent(FunctionAndUtil.Function, FuncParams->GetStructMemory());
	//										}
	//									}
	//									else
	//									{
	//										FScopedTransaction Transaction(NSLOCTEXT("UnrealEd", "BlutilityAction", "Blutility Action"));
	//										FEditorScriptExecutionGuard ScriptGuard;
	//										TempObject->ProcessEvent(FunctionAndUtil.Function, nullptr);
	//									}

	//									TempObject->RemoveFromRoot();
	//								}
	//							}));
	//				}
	//			}),
	//		false,
	//				FSlateIcon("EditorStyle", "GraphEditor.Event_16x"));
	//}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FWaypointsEditorExtensionModule_Impl, WaypointsEditorExtension)