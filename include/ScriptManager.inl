#ifndef __SCRIPTMANAGER_INL__
#define __SCRIPTMANAGER_INL__

void ScriptManager::RegisterScript(Hash hash, const std::string& name, ScriptFactory factory) {
	mScriptFactories[hash] = { name, factory };
}

CScript* ScriptManager::CreateScript(Hash hash) const {
	auto iter = mScriptFactories.find(hash);
	if (iter != mScriptFactories.end()) {
		const auto& info = iter->second;
		return info.Factory();
	}

	return nullptr;
}

#endif // __SCRIPTMANAGER_INL__