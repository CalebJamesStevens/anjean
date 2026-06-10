#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace Anjean {

std::string makeCsproj(const std::string& projectName)
{
    return
        "<Project Sdk=\"Microsoft.NET.Sdk\">\n"
        "\n"
        "  <PropertyGroup>\n"
        "    <TargetFramework>net8.0</TargetFramework>\n"
        "    <OutputType>Library</OutputType>\n"
        "    <Nullable>enable</Nullable>\n"
        "    <ImplicitUsings>enable</ImplicitUsings>\n"
        "    <RootNamespace>" + projectName + "</RootNamespace>\n"
        "  </PropertyGroup>\n"
        "\n"
        "  <ItemGroup>\n"
        "    <Reference Include=\"Anjean.Scripting\">\n"
        "      <HintPath>$(MSBuildProjectDirectory)/.anjean/sdk/Anjean.Scripting.dll</HintPath>\n"
        "      <Private>false</Private>\n"
        "    </Reference>\n"
        "  </ItemGroup>\n"
        "\n"
        "  <ItemGroup>\n"
        "    <Compile Include=\".anjean/generated/**/*.cs\" />\n"
        "  </ItemGroup>\n"
        "\n"
        "  <Target Name=\"CheckAnjeanSdk\" BeforeTargets=\"ResolveReferences\">\n"
        "    <Error\n"
        "      Condition=\"!Exists('$(MSBuildProjectDirectory)/.anjean/sdk/Anjean.Scripting.dll')\"\n"
        "      Text=\"Missing Anjean SDK. Run anjean restore or anjean init to install the scripting SDK.\" />\n"
        "  </Target>\n"
        "\n"
        "</Project>\n";
}
std::string makeDirectoryBuildProps()
{
    return
        "<Project>\n"
        "  <PropertyGroup>\n"
        "    <BaseOutputPath>.anjean/bin/</BaseOutputPath>\n"
        "    <BaseIntermediateOutputPath>.anjean/obj/</BaseIntermediateOutputPath>\n"
        "    <MSBuildProjectExtensionsPath>.anjean/obj/</MSBuildProjectExtensionsPath>\n"
        "\n"
        "    <DefaultItemExcludes>\n"
        "      $(DefaultItemExcludes);\n"
        "      .anjean/bin/**;\n"
        "      .anjean/obj/**\n"
        "    </DefaultItemExcludes>\n"
        "  </PropertyGroup>\n"
        "</Project>\n";
}

std::string makeGitignore()
{
    return
        ".anjean/bin/\n"
        ".anjean/obj/\n"
        "bin/\n"
        "obj/\n";
}

void writeFile(const std::filesystem::path& path, const std::string& content)
{
    std::ofstream file(path);

    if (!file)
    {
        throw std::runtime_error("Failed to write file: " + path.string());
    }

    file << content;
}

std::string getProjectNameFromPath(const std::filesystem::path& projectRoot)
{
    namespace fs = std::filesystem;

    fs::path normalized = fs::absolute(projectRoot).lexically_normal();
    std::string projectName = normalized.filename().string();

    if (projectName.empty())
    {
        throw std::runtime_error("Could not determine project name from path: " + projectRoot.string());
    }

    return projectName;
}

void initProjectAt(const std::filesystem::path& projectRoot, const std::string& projectName)
{
    namespace fs = std::filesystem;

    fs::path scriptingDll =
        "/Users/caleb/repos/anjean/src/scripting/Anjean.Scripting/bin/Debug/net8.0/Anjean.Scripting.dll";

    fs::path scriptingRuntimeConfig =
        "/Users/caleb/repos/anjean/src/scripting/Anjean.Scripting/bin/Debug/net8.0/Anjean.Scripting.runtimeconfig.json";

    if (!fs::exists(scriptingDll))
    {
        throw std::runtime_error(
            "Missing Anjean.Scripting.dll. Run dotnet build in Anjean.Scripting first."
        );
    }

    if (!fs::exists(scriptingRuntimeConfig))
    {
        throw std::runtime_error(
            "Missing Anjean.Scripting.runtimeconfig.json. Run dotnet build in Anjean.Scripting first."
        );
    }

    fs::create_directories(projectRoot);
    fs::create_directories(projectRoot / "Scripts");
    fs::create_directories(projectRoot / "Assets");
    fs::create_directories(projectRoot / ".anjean");
    fs::create_directories(projectRoot / ".anjean/sdk");
    fs::create_directories(projectRoot / ".anjean/generated");
    fs::create_directories(projectRoot / ".anjean/bin");
    fs::create_directories(projectRoot / ".anjean/obj");

    writeFile(projectRoot / (projectName + ".csproj"), makeCsproj(projectName));
    writeFile(projectRoot / "Directory.Build.props", makeDirectoryBuildProps());
    writeFile(projectRoot / ".gitignore", makeGitignore());

    writeFile(
        projectRoot / ".anjean/generated/GlobalUsings.cs",
        "global using System;\n"
        "global using Anjean;\n"
    );

    fs::copy_file(
      scriptingRuntimeConfig,
      projectRoot / ".anjean/sdk/Anjean.Scripting.runtimeconfig.json",
      fs::copy_options::overwrite_existing
    );

    fs::copy_file(
        scriptingDll,
        projectRoot / ".anjean/sdk/Anjean.Scripting.dll",
        fs::copy_options::overwrite_existing
    );

    std::cout << "Created Anjean project: " << projectName << std::endl;
    std::cout << "Project path: " << fs::absolute(projectRoot).lexically_normal() << std::endl;
}

void initProject()
{
    namespace fs = std::filesystem;

    fs::path projectRoot = fs::current_path();
    std::string projectName = getProjectNameFromPath(projectRoot);

    initProjectAt(projectRoot, projectName);
}

void initProject(const std::string& projectPathArg)
{
    namespace fs = std::filesystem;

    fs::path projectRoot;

    if (projectPathArg.empty() || projectPathArg == ".")
    {
        projectRoot = fs::current_path();
    }
    else
    {
        projectRoot = projectPathArg;
    }

    std::string projectName = getProjectNameFromPath(projectRoot);

    initProjectAt(projectRoot, projectName);
}
}