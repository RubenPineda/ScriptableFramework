// Copyright 2026 kirzo

#include "ScriptableFrameworkEd/Debug/ScriptableDebugRegistry.h"

#include "ScriptableNodes/ScriptableGraph.h"
#include "ScriptableNodes/ScriptableGraphInstance.h"

namespace FScriptableDebugRegistry
{
	/** Function-local statics so the symbol can't be accessed before first use and survives module reloads cleanly. */
	static TMap<TWeakObjectPtr<const UScriptableGraph>, TWeakObjectPtr<UScriptableGraphInstance>>& GetMap()
	{
		static TMap<TWeakObjectPtr<const UScriptableGraph>, TWeakObjectPtr<UScriptableGraphInstance>> Map;
		return Map;
	}

	void SetDebugInstance(const UScriptableGraph* Asset, UScriptableGraphInstance* Instance)
	{
		if (!Asset) return;
		if (Instance)
		{
			GetMap().Add(Asset, Instance);
		}
		else
		{
			GetMap().Remove(Asset);
		}
	}

	UScriptableGraphInstance* GetDebugInstance(const UScriptableGraph* Asset)
	{
		if (!Asset) return nullptr;
		const TWeakObjectPtr<UScriptableGraphInstance>* Found = GetMap().Find(Asset);
		return Found ? Found->Get() : nullptr;
	}
}
