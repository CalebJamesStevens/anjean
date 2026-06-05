using System.Reflection;
using System.Runtime.InteropServices;
using System.Runtime.Loader;

namespace Anjean;

public static class ScriptHost
{
    static ScriptHost()
    {
        NativeLibraryResolver.Initialize();
    }
    private static readonly Dictionary<string, Type> ScriptTypes = new();
    private static readonly Dictionary<string, Type> SceneTypes = new();
    private static readonly List<GameObject> ActiveObjects = new();


    [UnmanagedCallersOnly]
    public static int LoadGameAssembly(IntPtr assemblyPathPtr)
    {
        string? assemblyPath = Marshal.PtrToStringUTF8(assemblyPathPtr);

        Console.WriteLine($"Loading game assembly: {assemblyPath}");

        if (string.IsNullOrWhiteSpace(assemblyPath))
            return -1;

        AssemblyLoadContext.Default.Resolving += (_, assemblyName) =>
        {
            if (assemblyName.Name == "Anjean.Scripting")
            {
                return typeof(GameObject).Assembly;
            }

            return null;
        };

        Assembly assembly = Assembly.LoadFrom(assemblyPath);

        foreach (Type type in assembly.GetTypes())
        {
            Console.WriteLine($"Found type: {type.FullName}, Base: {type.BaseType?.FullName}");
            Console.WriteLine($"Host GameObject assembly: {typeof(GameObject).Assembly.Location}");
            Console.WriteLine($"Player base assembly: {type.BaseType?.Assembly.Location}");

            if (type.IsAbstract)
                continue;
            
            if (typeof(SceneDefinition).IsAssignableFrom(type))
            {
              Console.WriteLine($"Registering scene: {type.Name}");
              SceneTypes[type.Name] = type;
            }

            if (typeof(GameObject).IsAssignableFrom(type))
            {
              Console.WriteLine($"Registering script: {type.Name}");
              ScriptTypes[type.Name] = type;
            }

        }

        Console.WriteLine($"Total scripts registered: {ScriptTypes.Count}");
        return 0;
    }
    [UnmanagedCallersOnly]
    public static int CreateGameObjectScript(IntPtr scriptNamePtr, uint nativeObjectId)
    {
        string? scriptName = Marshal.PtrToStringUTF8(scriptNamePtr);

        Console.WriteLine($"Creating script: {scriptName}");

        if (string.IsNullOrWhiteSpace(scriptName))
            return -1;

        if (!ScriptTypes.TryGetValue(scriptName, out Type? scriptType))
        {
            Console.WriteLine("Known scripts:");

            foreach (string key in ScriptTypes.Keys)
            {
                Console.WriteLine($"- {key}");
            }

            return -2;
        }

        var script = (GameObject)Activator.CreateInstance(scriptType)!;
        script.Attach(nativeObjectId);

        ActiveObjects.Add(script);

        script.Start();

        return 0;
    }

    [UnmanagedCallersOnly]
    public static void UpdateAll()
    {
        foreach (GameObject obj in ActiveObjects)
        {
            obj.Update();
        }
    }
    
    [UnmanagedCallersOnly]
    public static int StartMainScene()
    {
        if (!SceneTypes.TryGetValue("Main", out Type? sceneType))
        {

            return -2;
        }

        var scene = (SceneDefinition)Activator.CreateInstance(sceneType)!;
        
        foreach (SceneObjectDefinition sceneObject in scene.StartUp())
        {
            int rc = Native.Anjean_Runtime_CreateGameObject(out uint runtimeId);

            if (rc != 0)
            {
                return rc;
            }

            var gameObject = (GameObject)Activator.CreateInstance(sceneObject.ObjectType)!;

            gameObject.Attach(runtimeId, sceneObject.Props);
            ActiveObjects.Add(gameObject);

            gameObject.Start();
        }

        return 0;
    }


}