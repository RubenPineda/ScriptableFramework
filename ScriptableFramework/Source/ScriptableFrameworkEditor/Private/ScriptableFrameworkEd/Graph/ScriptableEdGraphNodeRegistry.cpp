// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNodeRegistry.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode.h"
#include "ScriptableNodes/ScriptableNode.h"

#include "UObject/UObjectIterator.h"

TMap<UClass*, UClass*>& FScriptableEdGraphNodeRegistry::GetMap()
{
	static TMap<UClass*, UClass*> Map;
	return Map;
}

void FScriptableEdGraphNodeRegistry::Build()
{
	TMap<UClass*, UClass*>& Map = GetMap();
	Map.Reset();

	// Walk every UScriptableEdGraphNode subclass currently loaded. CDOs are guaranteed to exist
	// by the time module startup runs, so reading the RuntimeNodeClass field gives us each
	// concrete ed-node's declared mapping in one pass.
	for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
	{
		UClass* Cls = *ClassIt;
		if (!Cls || !Cls->IsChildOf(UScriptableEdGraphNode::StaticClass())) continue;
		if (Cls->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists)) continue;

		const UScriptableEdGraphNode* CDO = Cls->GetDefaultObject<UScriptableEdGraphNode>();
		if (!CDO || !CDO->RuntimeNodeClass) continue;

		// First registration wins. In practice every runtime class maps to exactly one ed-node;
		// a clash here means a coding mistake (two ed-nodes both claiming the same runtime).
		if (!Map.Contains(CDO->RuntimeNodeClass))
		{
			Map.Add(CDO->RuntimeNodeClass, Cls);
		}
	}
}

UClass* FScriptableEdGraphNodeRegistry::FindEdNodeClassFor(const UScriptableNode* RuntimeNode)
{
	if (!RuntimeNode) return nullptr;

	const TMap<UClass*, UClass*>& Map = GetMap();

	// Walk up the runtime class hierarchy in case the user has subclassed (e.g. their own Task
	// subclass should still resolve to the registered Task ed-node). Stop at UScriptableNode.
	UClass* Cls = RuntimeNode->GetClass();
	while (Cls && Cls != UScriptableNode::StaticClass())
	{
		if (UClass* const* Found = Map.Find(Cls))
		{
			return *Found;
		}
		Cls = Cls->GetSuperClass();
	}

	return nullptr;
}