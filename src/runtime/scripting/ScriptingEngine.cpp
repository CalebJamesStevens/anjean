#include "ScriptingEngine.h"

#include <coreclr_delegates.h>
#include <hostfxr.h>

#include <dlfcn.h>
#include <iostream>
#include <stdexcept>

namespace Anjean::Runtime
{
    namespace
    {
        hostfxr_initialize_for_runtime_config_fn hostfxr_initialize = nullptr;
        hostfxr_get_runtime_delegate_fn hostfxr_get_delegate = nullptr;
        hostfxr_close_fn hostfxr_close_ptr = nullptr;

        void loadHostFxr()
        {
            const char* hostfxrPath =
                "/usr/local/share/dotnet/host/fxr/8.0.1/libhostfxr.dylib";

            void* lib = dlopen(hostfxrPath, RTLD_LAZY | RTLD_LOCAL);

            if (!lib)
            {
                throw std::runtime_error(dlerror());
            }

            hostfxr_initialize =
                reinterpret_cast<hostfxr_initialize_for_runtime_config_fn>(
                    dlsym(lib, "hostfxr_initialize_for_runtime_config")
                );

            hostfxr_get_delegate =
                reinterpret_cast<hostfxr_get_runtime_delegate_fn>(
                    dlsym(lib, "hostfxr_get_runtime_delegate")
                );

            hostfxr_close_ptr =
                reinterpret_cast<hostfxr_close_fn>(
                    dlsym(lib, "hostfxr_close")
                );

            if (!hostfxr_initialize || !hostfxr_get_delegate || !hostfxr_close_ptr)
            {
                throw std::runtime_error("Failed to load hostfxr functions.");
            }
        }

        load_assembly_and_get_function_pointer_fn getLoadAssemblyFn(
            const std::string& runtimeConfigPath
        )
        {
            hostfxr_handle context = nullptr;

            int rc = hostfxr_initialize(
                runtimeConfigPath.c_str(),
                nullptr,
                &context
            );

            if (rc != 0 || context == nullptr)
            {
                throw std::runtime_error("hostfxr_initialize_for_runtime_config failed.");
            }

            void* loadAssembly = nullptr;

            rc = hostfxr_get_delegate(
                context,
                hdt_load_assembly_and_get_function_pointer,
                &loadAssembly
            );

            hostfxr_close_ptr(context);

            if (rc != 0 || loadAssembly == nullptr)
            {
                throw std::runtime_error("hostfxr_get_runtime_delegate failed.");
            }

            return reinterpret_cast<load_assembly_and_get_function_pointer_fn>(
                loadAssembly
            );
        }
    }

    void ScriptingEngine::bindRuntime(Runtime* pRuntime)
    {
        runtime = pRuntime;
    }

    void ScriptingEngine::load(
        const std::string& runtimeConfigPath,
        const std::string& scriptingDllPath
    )
    {
        loadHostFxr();

        auto loadAssemblyAndGetFunctionPointer =
            getLoadAssemblyFn(runtimeConfigPath);

        int rc = 0;

        rc = loadAssemblyAndGetFunctionPointer(
            scriptingDllPath.c_str(),
            "Anjean.ScriptHost, Anjean.Scripting",
            "LoadGameAssembly",
            UNMANAGEDCALLERSONLY_METHOD,
            nullptr,
            reinterpret_cast<void**>(&loadGameAssemblyFn)
        );

        if (rc != 0 || !loadGameAssemblyFn)
        {
            throw std::runtime_error("Failed to get ScriptHost.LoadGameAssembly.");
        }

        rc = loadAssemblyAndGetFunctionPointer(
            scriptingDllPath.c_str(),
            "Anjean.ScriptHost, Anjean.Scripting",
            "CreateGameObjectScript",
            UNMANAGEDCALLERSONLY_METHOD,
            nullptr,
            reinterpret_cast<void**>(&createGameObjectScriptFn)
        );

        if (rc != 0 || !createGameObjectScriptFn)
        {
            throw std::runtime_error("Failed to get ScriptHost.CreateGameObjectScript.");
        }

        rc = loadAssemblyAndGetFunctionPointer(
            scriptingDllPath.c_str(),
            "Anjean.ScriptHost, Anjean.Scripting",
            "StartMainScene",
            UNMANAGEDCALLERSONLY_METHOD,
            nullptr,
            reinterpret_cast<void**>(&startMainSceneFn)
        );

        if (rc != 0 || !startMainSceneFn)
        {
            throw std::runtime_error("Failed to get ScriptHost.StartMainScene.");
        }

        std::cout << "Loaded C# scripting host\n";

        rc = loadAssemblyAndGetFunctionPointer(
            scriptingDllPath.c_str(),
            "Anjean.ScriptHost, Anjean.Scripting",
            "UpdateAll",
            UNMANAGEDCALLERSONLY_METHOD,
            nullptr,
            reinterpret_cast<void**>(&updateAllFn)
        );

        if (rc != 0 || !updateAllFn)
        {
            throw std::runtime_error("Failed to get ScriptHost.UpdateAll.");
        }

        std::cout << "Loaded C# scripting host\n";
    }

    void ScriptingEngine::loadGameAssembly(const std::string& gameAssemblyPath)
    {
        if (!loadGameAssemblyFn)
        {
            throw std::runtime_error("ScriptingEngine not loaded.");
        }

        int rc = loadGameAssemblyFn(gameAssemblyPath.c_str());

        if (rc != 0)
        {
            throw std::runtime_error("Failed to load game assembly.");
        }
    }

    void ScriptingEngine::createGameObjectScript(
        const std::string& scriptName,
        std::uint32_t nativeObjectId
    )
    {
        if (!createGameObjectScriptFn)
        {
            throw std::runtime_error("ScriptingEngine not loaded.");
        }

        int rc = createGameObjectScriptFn(scriptName.c_str(), nativeObjectId);

        if (rc != 0)
        {
            throw std::runtime_error(
                "Failed to create script: " + scriptName +
                ", rc = " + std::to_string(rc)
            );
        }
    }

    void ScriptingEngine::updateAll()
    {
        if (updateAllFn)
        {
            updateAllFn();
        }
    }
    
    void ScriptingEngine::startMainScene()
    {
         if (!startMainSceneFn)
        {
            throw std::runtime_error("ScriptingEngine not loaded.");
        }

        int rc = startMainSceneFn();

        if (rc != 0)
        {
            throw std::runtime_error(
                "Failed to start main scene, rc = " + std::to_string(rc)
            );
        }
    }
}