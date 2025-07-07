#pragma once
template <typename ActorType>
class SequenceState : public StateBase<ActorType> {
public:
    SequenceState(ActorType* actor) : StateBase<ActorType>(actor) {}

    void AddChild(StateBase<ActorType>* child) {
        children.push_back(child);
    }

    void Enter() override {
        currentIndex = 0;
        if (!children.empty()) {
            children[currentIndex]->Enter();
        }
        isComplete = false;
        isFailed = false;
    }

    void Execute(float elapsedTime) override {
        if (currentIndex >= children.size()) {
            isComplete = true;
            return;
        }
        auto* current = children[currentIndex];
        current->Execute(elapsedTime);

        if (current->IsFailed()) {
            isFailed = true;
            return;
        }
        if (current->IsComplete()) {
            currentIndex++;
            if (currentIndex < children.size()) {
                children[currentIndex]->Enter();
            }
            else {
                isComplete = true;
            }
        }
    }

    bool IsComplete() const override { return isComplete; }
    bool IsFailed() const override { return isFailed; }
    const char* GetName() const override { return "SequenceState"; }

private:
    std::vector<StateBase<ActorType>*> children;
    size_t currentIndex = 0;
    bool isComplete = false;
    bool isFailed = false;
};