#pragma once

#include "Singleton.hpp"
#include "CScript.hpp"

class ScriptManager : public Singleton<ScriptManager> {
	SINGLETON(ScriptManager);

public:
	__forceinline void RegisterScript(Hash hash, const std::string& name, ScriptFactory factory);

	__forceinline CScript* CreateScript(Hash hash) const;

	const std::string& GetScriptName(Hash hash) const;
	const auto& GetScriptNames() const noexcept;

private:
	std::unordered_map<Hash, ScriptInfo> mScriptFactories;
};

#include "ScriptManager.inl"

#ifndef SCRIPT_MANAGER
#define SCRIPT_MANAGER ScriptManager::GetInstance()
#endif // SCRIPT_MANAGER

#define DECLARE_SCRIPT(__Type)										\
public:																\
    static ScriptID StaticID() {									\
        static ScriptID id = std::hash<std::string>{}(#__Type);		\
        return id;													\
    }																\
    virtual ScriptID GetID() const override { return StaticID(); }

#define REGISTER_SCRIPT(__Type)                                                 \
    namespace {                                                                 \
        struct __Type##Register {                                               \
            __Type##Register() {                                                \
                SCRIPT_MANAGER->Register(                                       \
                    __Type()->GetID(), #__Type, []() { return new __Type(); }); \
            }                                                                   \
        };                                                                      \
        static __Type##Register global_##__Type##Register;                      \
    }