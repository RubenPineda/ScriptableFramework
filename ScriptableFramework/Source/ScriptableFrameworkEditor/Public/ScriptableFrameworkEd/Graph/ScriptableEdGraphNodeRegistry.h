// Copyright 2026 kirzo

#pragma once

#include "CoreMinimal.h"

class UScriptableNode;
class UScriptableEdGraphNode;
enum class EModuleChangeReason;

/**
 * Lookup table: runtime UScriptableNode class -> visual UScriptableEdGraphNode class.
 *
 * Fed by two sources: reflection auto-discovery (every concrete UScriptableEdGraphNode subclass that
 * sets RuntimeNodeClass) and explicit RegisterEdNodeClass calls. The reflected set is rebuilt whenever
 * modules load/unload, so ed-nodes from plugins loaded after startup are picked up; explicit
 * registrations persist across rebuilds and take precedence.
 */
class SCRIPTABLEFRAMEWORKEDITOR_API FScriptableEdGraphNodeRegistry
{
public:
	/** Runs the initial reflection scan and starts tracking module changes. Call once at editor module startup; idempotent. */
	static void Initialize();

	/** Stops tracking module changes and clears all entries. Call at editor module shutdown. */
	static void Shutdown();

	/**
	 * Explicitly maps a runtime node class to its ed-node class. For external modules whose ed-nodes
	 * don't (or can't) declare RuntimeNodeClass for reflection. Call from the module's StartupModule;
	 * pair with UnregisterEdNodeClass in ShutdownModule. Overrides any reflection-discovered mapping.
	 */
	static void RegisterEdNodeClass(UClass* RuntimeNodeClass, UClass* EdNodeClass);

	/** Removes an explicit mapping previously added via RegisterEdNodeClass. */
	static void UnregisterEdNodeClass(UClass* RuntimeNodeClass);

	/** Returns the ed-node class for the given runtime instance (walking up its hierarchy), or null — caller falls back to the generic native ed-node. */
	static UClass* FindEdNodeClassFor(const UScriptableNode* RuntimeNode);

private:
	/** Rebuilds the reflected set from all currently-loaded UScriptableEdGraphNode subclasses. */
	static void RebuildReflectedClasses();

	/** Module load/unload hook: rebuilds the reflected set (explicit registrations are untouched). */
	static void OnModulesChanged(FName ModuleName, EModuleChangeReason Reason);

	/** Explicit registrations. Persist across reflection rebuilds and win over reflected entries. */
	static TMap<UClass*, UClass*>& GetExplicitMap();

	/** Reflection-discovered registrations. Rebuilt on module changes. */
	static TMap<UClass*, UClass*>& GetReflectedMap();
};
