# Scriptable Framework for Unreal Engine

**Scriptable Framework** is a data-driven logic system for Unreal Engine. It empowers designers and programmers to embed modular logic sequences (**Actions**) and validation rules (**Requirements**) directly into Actors, Data Assets, or any `UObject`, without the overhead of creating unique Blueprint graphs for every specific interaction.

The system relies on a robust **Runtime Binding System** that allows properties within tasks and conditions to be dynamically bound to context variables defined by the host object, ensuring strict type safety and validation at editor time.

---

## 🌟 Key Features

### Embeddable Logic
Add `FScriptableAction` or `FScriptableRequirement` properties to your classes to instantly expose a logic editor in the **Details Panel**.

### Modular Architecture

- **Actions**  
  Sequences of Tasks (e.g., *Play Sound*, *Spawn Actor*, *Wait*).

- **Requirements**  
  Lists of Conditions (e.g., *Is Alive?*, *Has Ammo?*) evaluated with **AND / OR** logic.

### Typed Context Definition
Define the **Signature** (inputs) of your logic containers via C++.  
The editor uses this information to provide only valid binding options.

### Advanced Data Binding

- Bind properties inside your nodes to the defined **Context** variables.
- **Editor Validation** automatically detects:
  - Missing context parameters
  - Type mismatches (e.g., binding a `float` to a `bool`)
- **Deep Access**: supports binding to nested struct properties.

### No-Graph Editor
A clean, vertical list interface using heavily customized **Detail Views**.  
It feels like editing standard properties but behaves like a logic flow.

---

## 📦 Dependencies

- **Unreal Engine 5.5+**

---

## 🚀 Core Concepts

### 1. The Containers (The Core)

The framework revolves around two main structs that you embed in your classes:

- **`FScriptableAction`**  
  Holds and executes a sequence of `UScriptableTask`.

<p align="center">
	<img src="https://kirzo.dev/content/images/plugins/ScriptableFramework/action_loop.jpg" width="512">
</p>

- **`FScriptableRequirement`**  
  Holds and evaluates a set of `UScriptableCondition`.

<p align="center">
	<img src="https://kirzo.dev/content/images/plugins/ScriptableFramework/nested_requirement.jpg" width="512">
</p>

---

### 2. The Nodes

- **Task (`UScriptableTask`)**  
  The atomic unit of work. Executes logic.

- **Condition (`UScriptableCondition`)**  
  The atomic unit of logic. Checks a state and returns a boolean.

---

### 3. The Assets (Convenience)

- **ScriptableActionAsset**
- **ScriptableRequirementAsset**

Data Assets that wrap the containers.  
Useful for defining reusable logic shared across multiple objects, although the system is designed primarily for containers embedded directly in Actors.

---

## 💻 C++ Integration Guide

To use the framework, add the containers as properties to your Actor (or any `UObject`).  
You define the **Context** (the variables available for binding in the editor) inside `PostEditChangeProperty`.

At runtime, you are responsible for providing the actual data to the Context before executing an Action or evaluating a Requirement. The framework then resolves all bindings and runs the configured logic or conditions.

---

### Example: Embedding Logic in an Actor

```cpp
// .h
UCLASS()
class AMyActor : public AActor
{
    GENERATED_BODY()

public:
    // A standard property we want to expose to the logic
    UPROPERTY(EditAnywhere, Category = "MyActor")
    float Health = 100.0f;

    // A logic sequence (e.g., what happens when this actor dies)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MyActor")
    FScriptableAction OnDeathAction;

    // A condition check (e.g., can this actor be interacted with?)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "MyActor")
    FScriptableRequirement InteractionRequirement;

#if WITH_EDITOR
    // Defines the "Signature" of the context for the Editor
    virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
#endif

    // Runtime execution
    void KillActor();
    bool CanInteract();
};

// .cpp
#if WITH_EDITOR
void AMyActor::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
    Super::PostEditChangeProperty(PropertyChangedEvent);

    // Update the Action Context definition
    if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(AMyActor, OnDeathAction))
    {
        OnDeathAction.ResetContext();
        OnDeathAction.AddContextProperty<float>(TEXT("Health"));
        // e.g.: .AddContextProperty<AActor*>(TEXT("Instigator"));
    }
    // Update the Requirement Context definition
    else if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(AMyActor, InteractionRequirement))
    {
        InteractionRequirement.ResetContext();
        InteractionRequirement.AddContextProperty<float>(TEXT("Health"));
    }
}
#endif

void AMyActor::KillActor()
{
    // 1. Pass runtime data to the context
    OnDeathAction.SetContextProperty<float>(TEXT("Health"), Health);

    // 2. Run the action
    FScriptableAction::RunAction(this, OnDeathAction);
}

bool AMyActor::CanInteract()
{
    // 1. Pass runtime data
    InteractionRequirement.SetContextProperty<float>(TEXT("Health"), Health);

    // 2. Evaluate
    return FScriptableRequirement::EvaluateRequirement(this, InteractionRequirement);
}
```

## 🛠 Extending the Framework

### Creating Custom Tasks

To extend the system with new behavior, create new Tasks by inheriting from `UScriptableTask`. The `Context` is used internally to resolve bindings before `BeginTask()` is called, so your task simply uses its own properties.

```cpp
UCLASS(DisplayName = "Print Message", meta = (TaskCategory = "System"))
class UMyTask_Print : public UScriptableTask
{
	GENERATED_BODY()

public:
	// User binds this property in the Editor to a Context variable (e.g., "PlayerName")
	UPROPERTY(EditAnywhere, Category = "Config")
	FString Message = "Default";

protected:
	virtual void BeginTask() override
	{
		// 'Message' already contains the resolved value from the Context here.
		GEngine->AddOnScreenDebugMessage(-1, 2.0f, FColor::Cyan, Message);

		Finish(); // Call Finish() to notify owner that we are done.
	}
};
```

<p align="center">
	<img src="https://kirzo.dev/content/images/plugins/ScriptableFramework/action_1.jpg" width="512">
	<img src="https://kirzo.dev/content/images/plugins/ScriptableFramework/action_2.jpg" width="512">
</p>

### Creating Custom Conditions

To add new validation logic, create Conditions by inheriting from `UScriptableCondition`.

Conditions are pure logic checks that:
- Read data from their bound properties
- Evaluate a state
- Return a boolean result

They are designed to be simple and reusable.

```cpp
UCLASS(DisplayName = "Is Health Low", meta = (ConditionCategory = "Gameplay|Health"))
class UMyCondition_IsHealthLow : public UScriptableCondition
{
	GENERATED_BODY()

public:
	// Bind this to the "Health" float in the Context
	UPROPERTY(EditAnywhere, Category = "Config")
	float CurrentHealth = 0.0f;

	UPROPERTY(EditAnywhere, Category = "Config")
	float Threshold = 20.0f;

protected:
	virtual bool Evaluate_Implementation() const override
	{
		return CurrentHealth < Threshold;
	}
};
```

<p align="center">
	<img src="https://kirzo.dev/content/images/plugins/ScriptableFramework/requirement_1.jpg" width="512">
</p>


## Organization & Filtering

To keep your editor clean, you can organize your custom nodes into submenus in the picker using metadata.

```cpp
// This will appear under "System" in the dropdown
UCLASS(DisplayName = "Print Message", meta = (TaskCategory = "System"))
class UMyTask_Print : public UScriptableTask { ... };

// This will appear under "Gameplay > Health"
UCLASS(DisplayName = "Is Health Low", meta = (ConditionCategory = "Gameplay|Health"))
class UMyCondition_IsHealthLow : public UScriptableCondition { ... };
```

You can also restrict which categories are allowed for a specific property. This is useful if you want a Requirement field to only accept specific types of conditions (e.g., only **"Gameplay"** conditions).

```cpp
// Only Tasks with Category="Gameplay" (or subcategories) will be selectable here
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = MyActor, meta = (TaskCategories = "Gameplay"))
FScriptableAction GameplayAction;

// Only Conditions with Category="Gameplay" (or subcategories) will be selectable here
UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = MyActor, meta = (ConditionCategories = "Gameplay"))
FScriptableRequirement GameplayRequirement;
```

<p align="center">
	<img src="https://kirzo.dev/content/images/plugins/ScriptableFramework/category_filter.jpg" width="512">
</p>
