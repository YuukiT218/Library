#pragma once

//#include "Character/Enemy/Enemy.h"

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

	void ReversLockOnSwitch() { isLockOn = !isLockOn; }
	void SetIsLockOn(bool islockon) { isLockOn = islockon; }
	bool GetIsLockOn() { return isLockOn; }

	void SetLockOnEnemy(Enemy* enemy) { closestEnemy = enemy; }
	Enemy* GetLockOnEnemy() { return closestEnemy; }

private:
	bool isLockOn = false;
	Enemy* closestEnemy = nullptr;
};
