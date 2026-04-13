#include "pch.h"
#include "ScriptManager.hpp"

ScriptManager::ScriptManager() {}

ScriptManager::~ScriptManager() {}

const std::string& ScriptManager::GetScriptName(Hash hash) const {
	const auto iter = mScriptFactories.find(hash);
	assert(iter != mScriptFactories.end());

	return iter->second.Name;
}

void ScriptManager::GetScriptNames(std::vector<std::string>& names) const noexcept {
	for (const auto& pair : mScriptFactories) {
		const auto& info = pair.second;
		names.push_back(info.Name);
	}
}