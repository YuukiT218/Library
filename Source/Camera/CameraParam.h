#pragma once

#include "Character/Enemy/Enemy.h"

class CameraParam
{
public:
	CameraParam() {}
	~CameraParam() {}

public:
	// 唯一のインスタンス取得
	static CameraParam& Instance()
	{
		static CameraParam param;
		return param;
	}

	void ToggleLockOn() { isLockOn = !isLockOn; }
	void SetLockOn(bool isLockOn) { this->isLockOn = isLockOn; }
	bool IsLockOn() { return isLockOn; }

	void SetLockOnEnemy(Enemy* enemy) { closestEnemy = enemy; }
	Enemy* GetLockOnEnemy() { return closestEnemy; }

private:
	bool isLockOn = false;
	Enemy* closestEnemy = nullptr;
};
