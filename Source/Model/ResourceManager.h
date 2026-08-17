#pragma once

#include <memory>
#include <string>
#include <map>
#include "Model.h"

class ResourceManager
{
private:
	ResourceManager() {}
	~ResourceManager() {}

public:
	// 唯一のインスタンス
	static ResourceManager& Instance()
	{
		static ResourceManager instance;
		return instance;
	}

	// モデルリソース読み込み
	std::shared_ptr<ModelResource> LoadModelResource(const char* filename, float scale = 1.0f);

	void Clear();

private:
	using ModelMap = std::map<std::string, std::weak_ptr<ModelResource>>;

	ModelMap models;
};