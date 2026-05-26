// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNodeRegistry.h"
#include "ScriptableFrameworkEd/Graph/ScriptableEdGraphNode.h"
#include "ScriptableNodes/ScriptableNode.h"

#include "Modules/ModuleManager.h"
#include "UObject/UObjectIterator.h"

namespace
{
	FDelegateHandle GModulesChangedHandle;
}

TMap<UClass*, UClass*>& FScriptableEdGraphNodeRegistry::GetExplicitMap()
{
	static TMap<UClass*, UClass*> Map;
	return Map;
}

TMap<UClass*, UClass*>& FScriptableEdGraphNodeRegistry::GetReflectedMap()
{
	static TMap<UClass*, UClass*> Map;
	return Map;
}

void FScriptableEdGraphNodeRegistry::Initialize()
{
	RebuildReflectedClasses();

	// Rescan when modules load/unload so ed-nodes from plugins loaded after us are picked up.
	if (!GModulesChangedHandle.IsValid())
	{
		GModulesChangedHandle = FModuleManager::Get().OnModulesChanged().AddStatic(&FScriptableEdGraphNodeRegistry::OnModulesChanged);
	}
}

void FScriptableEdGraphNodeRegistry::Shutdown()
{
	if (GModulesChangedHandle.IsValid())
	{
		FModuleManager::Get().OnModulesChanged().Remove(GModulesChangedHandle);
		GModulesChangedHandle.Reset();
	}

	GetReflectedMap().Reset();
	GetExplicitMap().Reset();
}

void FScriptableEdGraphNodeRegistry::RebuildReflectedClasses()
{
	TMap<UClass*, UClass*>& Map = GetReflectedMap();
	Map.Reset();

	// Walk every loaded UScriptableEdGraphNode subclass; its CDO's RuntimeNodeClass is the declared
	// mapping. Rebuilding from the live iterator naturally drops classes from unloaded modules.
	for (TObjectIterator<UClass> ClassIt; ClassIt; ++ClassIt)
	{
		UClass* Cls = *ClassIt;
		if (!Cls || !Cls->IsChildOf(UScriptableEdGraphNode::StaticClass())) continue;
		if (Cls->HasAnyClassFlags(CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists)) continue;

		const UScriptableEdGraphNode* CDO = Cls->GetDefaultObject<UScriptableEdGraphNode>();
		if (!CDO || !CDO->RuntimeNodeClass) continue;

		// First registration wins; a clash means two ed-nodes claim the same runtime class.
		if (!Map.Contains(CDO->RuntimeNodeClass))
		{
			Map.Add(CDO->RuntimeNodeClass, Cls);
		}
	}
}

void FScriptableEdGraphNodeRegistry::OnModulesChanged(FName ModuleName, EModuleChangeReason Reason)
{
	if (Reason == EModuleChangeReason::ModuleLoaded || Reason == EModuleChangeReason::ModuleUnloaded)
	{
		RebuildReflectedClasses();
	}
}

void FScriptableEdGraphNodeRegistry::RegisterEdNodeClass(UClass* RuntimeNodeClass, UClass* EdNodeClass)
{
	if (!RuntimeNodeClass || !EdNodeClass) return;
	if (!EdNodeClass->IsChildOf(UScriptableEdGraphNode::StaticClass())) return;

	// Explicit intent overwrites any prior explicit entry for this runtime class.
	GetExplicitMap().Add(RuntimeNodeClass, EdNodeClass);
}

void FScriptableEdGraphNodeRegistry::UnregisterEdNodeClass(UClass* RuntimeNodeClass)
{
	if (!RuntimeNodeClass) return;
	GetExplicitMap().Remove(RuntimeNodeClass);
}

UClass* FScriptableEdGraphNodeRegistry::FindEdNodeClassFor(const UScriptableNode* RuntimeNode)
{
	if (!RuntimeNode) return nullptr;

	const TMap<UClass*, UClass*>& Explicit = GetExplicitMap();
	const TMap<UClass*, UClass*>& Reflected = GetReflectedMap();

	// Walk up the runtime class hierarchy (so a user subclass resolves to its ancestor's ed-node).
	// At each level explicit registrations win over reflected ones. Stop at UScriptableNode.
	UClass* Cls = RuntimeNode->GetClass();
	while (Cls && Cls != UScriptableNode::StaticClass())
	{
		if (UClass* const* Found = Explicit.Find(Cls)) return *Found;
		if (UClass* const* Found = Reflected.Find(Cls)) return *Found;
		Cls = Cls->GetSuperClass();
	}

	return nullptr;
}
