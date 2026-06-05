#pragma once

#include <cstdint>
#include <string>

namespace Anjean::Runtime
{
    class Runtime;

    class ScriptingEngine
    {
    public:
        void bindRuntime(Runtime* runtime);

        void load(
            const std::string& runtimeConfigPath,
            const std::string& scriptingDllPath
        );

        void loadGameAssembly(const std::string& gameAssemblyPath);

        void createGameObjectScript(
            const std::string& scriptName,
            std::uint32_t nativeObjectId
        );

        void updateAll();
        void startMainScene();

    private:
        Runtime* runtime = nullptr;

        using LoadGameAssemblyFn = int (*)(const char*);
        using StartMainSceneFn = int (*)();
        using CreateGameObjectScriptFn = int (*)(const char*, std::uint32_t);
        using UpdateAllFn = void (*)();

        LoadGameAssemblyFn loadGameAssemblyFn = nullptr;
        StartMainSceneFn startMainSceneFn = nullptr;
        CreateGameObjectScriptFn createGameObjectScriptFn = nullptr;
        UpdateAllFn updateAllFn = nullptr;
    };
}