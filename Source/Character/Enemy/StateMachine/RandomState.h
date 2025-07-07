#pragma once
#include <random>

template <typename ActorType>
class RandomState : public StateBase<ActorType> {
public:
    RandomState(ActorType* actor) : StateBase<ActorType>(actor) {}
    void AddChild(StateBase<ActorType>* child) {
        children.push_back(child);
        this->RegisterState(child);
    }
    void Enter() override {
        if (children.empty()) return;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, children.size() - 1);
        int idx = dis(gen);
        //this->currentState = children[idx];
        current = children[idx];
        current->Enter();
        //this->ChangeState(children[idx]->GetName());
        isComplete = false;
        isFailed = false;
    }
    void Execute(float elapsedTime) override {
        if (!current) return;
        current->Execute(elapsedTime);
        if (current->IsFailed()) isFailed = true;
        if (current->IsComplete()) isComplete = true;
    }
    bool IsComplete() const override { return isComplete; }
    bool IsFailed() const override { return isFailed; }
    const char* GetName() const override { return "RandomState"; }
private:
    std::vector<StateBase<ActorType>*> children;
    StateBase<ActorType>* current = nullptr;
    bool isComplete = false;
    bool isFailed = false;
};