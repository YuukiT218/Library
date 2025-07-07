#pragma once
#include <string>
#include <unordered_map>
#include "System\Misc.h"

template <typename ActorType>
class StateBase
{
public:
    // コンストラクタ
    StateBase(ActorType* actor) :owner(actor) {}
    virtual ~StateBase();
    // 全て継承先で実装させる必要があるため純粋仮想関数で実装
    // ステートに入った時のメソッド
    virtual void Enter() = 0;
    // ステートで実行するメソッド
    virtual void Execute(float elapsedTime) = 0;
    // ステートから出ていくときのメソッド
    virtual void Exit();

    // ステート取得
    StateBase<ActorType>* GetState()const { return currentState; }

    // ステート名取得
    virtual const char* GetName() const = 0;

    // サブステート変更
    virtual void ChangeState(const std::string& name);
    // サブステート登録
    virtual void RegisterState(StateBase<ActorType>* stateBase);

    virtual bool IsComplete() const { return false; }
    virtual bool IsFailed() const { return false; }

protected:
    ActorType* owner;
    // ステートリスト
    std::unordered_map<std::string, StateBase<ActorType>*> statePool;
    // 親のステート
    StateBase<ActorType>* parentState = nullptr;
    // 現在のステート
    StateBase<ActorType>* currentState = nullptr;
};

template <typename ActorType>
StateBase<ActorType>::~StateBase()
{
    for (auto& pair : statePool)
    {
        delete pair.second;
        pair.second = nullptr;
    }
    statePool.clear();
}

template <typename ActorType>
void StateBase<ActorType>::Exit()
{
    if (currentState != nullptr)
    {
        currentState->Exit();
        currentState = nullptr;
    }
}

template <typename ActorType>
void StateBase<ActorType>::ChangeState(const std::string& name)
{
    if (currentState != nullptr)
        currentState->Exit();

    auto it = statePool.find(name);
    if (it != statePool.end())
    {
        currentState = it->second;
        currentState->Enter();
    }
    else
    {
        currentState = nullptr;
    }
}

template <typename ActorType>
void StateBase<ActorType>::RegisterState(StateBase<ActorType>* stateBase)
{
    if (stateBase != nullptr)
    {
        stateBase->parentState = this;
        statePool[stateBase->GetName()] = stateBase;
    }
}