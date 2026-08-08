#include "Tools/MD_FoleyHapticsMigrationLibrary.h"

#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraphSchema_K2.h"
#include "Feedback/MD_FoleyEventBlueprintLibrary.h"
#include "K2Node_CallFunction.h"
#include "K2Node_FunctionEntry.h"
#include "K2Node_VariableGet.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "UObject/SavePackage.h"

namespace MDFoleyHapticsMigration
{
	constexpr TCHAR BlueprintPath[] =
		TEXT("/Game/Blueprints/AnimNotifies/BP_AnimNotify_FoleyEvent.BP_AnimNotify_FoleyEvent");
	constexpr TCHAR GraphName[] = TEXT("Received_Notify");

	UBlueprint* LoadFoleyBlueprint()
	{
		return LoadObject<UBlueprint>(nullptr, BlueprintPath);
	}

	UEdGraph* FindNotifyGraph(UBlueprint* Blueprint)
	{
		if (!Blueprint)
		{
			return nullptr;
		}

		for (UEdGraph* Graph : Blueprint->FunctionGraphs)
		{
			if (Graph && Graph->GetName() == GraphName)
			{
				return Graph;
			}
		}

		return nullptr;
	}

	UK2Node_CallFunction* FindCall(UEdGraph* Graph, const FName FunctionName)
	{
		if (!Graph)
		{
			return nullptr;
		}

		for (UEdGraphNode* Node : Graph->Nodes)
		{
			UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(Node);
			if (Call && Call->FunctionReference.GetMemberName() == FunctionName)
			{
				return Call;
			}
		}

		return nullptr;
	}

	UEdGraphPin* FindEntryOutput(UEdGraph* Graph, const FName PinName)
	{
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			if (UK2Node_FunctionEntry* Entry = Cast<UK2Node_FunctionEntry>(Node))
			{
				return Entry->FindPin(PinName, EGPD_Output);
			}
		}

		return nullptr;
	}

	UEdGraphPin* FindVariableOutput(UEdGraph* Graph, const FName VariableName)
	{
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			UK2Node_VariableGet* VariableGet = Cast<UK2Node_VariableGet>(Node);
			if (!VariableGet || VariableGet->VariableReference.GetMemberName() != VariableName)
			{
				continue;
			}

			for (UEdGraphPin* Pin : VariableGet->Pins)
			{
				if (Pin && Pin->Direction == EGPD_Output
					&& Pin->PinType.PinCategory != UEdGraphSchema_K2::PC_Exec)
				{
					return Pin;
				}
			}
		}

		return nullptr;
	}

	UK2Node_CallFunction* AddCallNode(
		UEdGraph* Graph,
		UFunction* Function,
		const int32 NodePosX,
		const int32 NodePosY)
	{
		FGraphNodeCreator<UK2Node_CallFunction> NodeCreator(*Graph);
		UK2Node_CallFunction* Node = NodeCreator.CreateNode();
		Node->SetFromFunction(Function);
		Node->NodePosX = NodePosX;
		Node->NodePosY = NodePosY;
		NodeCreator.Finalize();
		return Node;
	}

	bool InsertAfter(
		UEdGraph* Graph,
		UK2Node_CallFunction* SourceCall,
		UFunction* ReportFunction,
		UEdGraphPin* MeshCompPin,
		UEdGraphPin* EventTagPin,
		UEdGraphPin* PlaybackComponentPin,
		const int32 VerticalOffset)
	{
		if (!Graph || !SourceCall || !ReportFunction || !MeshCompPin || !EventTagPin)
		{
			return false;
		}

		const UEdGraphSchema_K2* Schema = GetDefault<UEdGraphSchema_K2>();
		UEdGraphPin* SourceThen = SourceCall->FindPin(UEdGraphSchema_K2::PN_Then, EGPD_Output);
		if (!SourceThen)
		{
			return false;
		}

		const TArray<UEdGraphPin*> PreviousDownstream = SourceThen->LinkedTo;
		SourceThen->BreakAllPinLinks();

		UK2Node_CallFunction* ReportCall = AddCallNode(
			Graph,
			ReportFunction,
			SourceCall->NodePosX + 360,
			SourceCall->NodePosY + VerticalOffset);

		UEdGraphPin* ReportExecute = ReportCall->FindPin(UEdGraphSchema_K2::PN_Execute, EGPD_Input);
		UEdGraphPin* ReportThen = ReportCall->FindPin(UEdGraphSchema_K2::PN_Then, EGPD_Output);
		UEdGraphPin* ReportMesh = ReportCall->FindPin(TEXT("MeshComp"), EGPD_Input);
		UEdGraphPin* ReportEvent = ReportCall->FindPin(TEXT("EventTag"), EGPD_Input);
		if (!ReportExecute || !ReportThen || !ReportMesh || !ReportEvent
			|| !Schema->TryCreateConnection(SourceThen, ReportExecute)
			|| !Schema->TryCreateConnection(MeshCompPin, ReportMesh)
			|| !Schema->TryCreateConnection(EventTagPin, ReportEvent))
		{
			return false;
		}

		if (PlaybackComponentPin)
		{
			UEdGraphPin* ReportPlayback =
				ReportCall->FindPin(TEXT("PlaybackComponent"), EGPD_Input);
			if (!ReportPlayback
				|| !Schema->TryCreateConnection(PlaybackComponentPin, ReportPlayback))
			{
				return false;
			}
		}

		for (UEdGraphPin* DownstreamPin : PreviousDownstream)
		{
			Schema->TryCreateConnection(ReportThen, DownstreamPin);
		}

		return true;
	}

	int32 CountReportCalls(UEdGraph* Graph)
	{
		if (!Graph)
		{
			return 0;
		}

		int32 Count = 0;
		for (UEdGraphNode* Node : Graph->Nodes)
		{
			const UK2Node_CallFunction* Call = Cast<UK2Node_CallFunction>(Node);
			if (!Call)
			{
				continue;
			}

			const FName Name = Call->FunctionReference.GetMemberName();
			Count += Name == GET_FUNCTION_NAME_CHECKED(
				UMD_FoleyEventBlueprintLibrary,
				ReportFoleyEventPlayed);
			Count += Name == GET_FUNCTION_NAME_CHECKED(
				UMD_FoleyEventBlueprintLibrary,
				ReportFoleyEventPlayedFromAudioComponent);
		}
		return Count;
	}
}

bool UMD_FoleyHapticsMigrationLibrary::IntegrateFoleyNotify()
{
	using namespace MDFoleyHapticsMigration;

	UBlueprint* Blueprint = LoadFoleyBlueprint();
	UEdGraph* Graph = FindNotifyGraph(Blueprint);
	if (!Blueprint || !Graph)
	{
		UE_LOG(LogTemp, Error, TEXT("Foley haptics migration: Blueprint or Received_Notify graph missing."));
		return false;
	}

	if (CountReportCalls(Graph) >= 2)
	{
		return true;
	}

	UK2Node_CallFunction* PlayFoleyEvent = FindCall(Graph, TEXT("PlayFoleyEvent"));
	UK2Node_CallFunction* PlaySound2D = FindCall(Graph, TEXT("PlaySound2D"));
	UEdGraphPin* MeshComp = FindEntryOutput(Graph, TEXT("MeshComp"));
	UEdGraphPin* EventTag = FindVariableOutput(Graph, TEXT("Event"));
	UEdGraphPin* AudioComponent = PlayFoleyEvent
		? PlayFoleyEvent->FindPin(TEXT("AudioComponent"), EGPD_Output)
		: nullptr;
	if (!PlayFoleyEvent || !PlaySound2D || !MeshComp || !EventTag || !AudioComponent)
	{
		UE_LOG(LogTemp, Error,
			TEXT("Foley haptics migration: expected playback nodes or data pins are missing."));
		return false;
	}

	UFunction* CheckedReportFunction =
		UMD_FoleyEventBlueprintLibrary::StaticClass()->FindFunctionByName(
			GET_FUNCTION_NAME_CHECKED(
				UMD_FoleyEventBlueprintLibrary,
				ReportFoleyEventPlayedFromAudioComponent));
	UFunction* FallbackReportFunction =
		UMD_FoleyEventBlueprintLibrary::StaticClass()->FindFunctionByName(
			GET_FUNCTION_NAME_CHECKED(
				UMD_FoleyEventBlueprintLibrary,
				ReportFoleyEventPlayed));

	Blueprint->Modify();
	Graph->Modify();
	const bool bNormalPathIntegrated = InsertAfter(
		Graph,
		PlayFoleyEvent,
		CheckedReportFunction,
		MeshComp,
		EventTag,
		AudioComponent,
		0);
	const bool bFallbackPathIntegrated = InsertAfter(
		Graph,
		PlaySound2D,
		FallbackReportFunction,
		MeshComp,
		EventTag,
		nullptr,
		160);
	if (!bNormalPathIntegrated || !bFallbackPathIntegrated)
	{
		UE_LOG(LogTemp, Error, TEXT("Foley haptics migration: failed to connect report nodes."));
		return false;
	}

	FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	if (Blueprint->Status == BS_Error || CountReportCalls(Graph) != 2)
	{
		UE_LOG(LogTemp, Error, TEXT("Foley haptics migration: Blueprint compile failed."));
		return false;
	}

	UPackage* Package = Blueprint->GetOutermost();
	const FString Filename = FPackageName::LongPackageNameToFilename(
		Package->GetName(),
		FPackageName::GetAssetPackageExtension());
	FSavePackageArgs SaveArgs;
	SaveArgs.TopLevelFlags = RF_Public | RF_Standalone;
	SaveArgs.SaveFlags = SAVE_NoError;
	const bool bSaved = UPackage::SavePackage(Package, Blueprint, *Filename, SaveArgs);
	UE_LOG(LogTemp, Display,
		TEXT("Foley haptics migration: integrated=%s saved=%s asset=%s"),
		CountReportCalls(Graph) == 2 ? TEXT("true") : TEXT("false"),
		bSaved ? TEXT("true") : TEXT("false"),
		BlueprintPath);
	return bSaved;
}

bool UMD_FoleyHapticsMigrationLibrary::IsFoleyNotifyIntegrated()
{
	using namespace MDFoleyHapticsMigration;
	UBlueprint* Blueprint = LoadFoleyBlueprint();
	return CountReportCalls(FindNotifyGraph(Blueprint)) == 2;
}
