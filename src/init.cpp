#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

namespace Anjean
{

std::string makeGitignore()
{
	return "bin/\n"
	       "obj/\n";
}

void writeFile(const std::filesystem::path &path, const std::string &content)
{
	std::ofstream file(path);

	if (!file)
	{
		throw std::runtime_error("Failed to write file: " + path.string());
	}

	file << content;
}

std::string getProjectNameFromPath(const std::filesystem::path &projectRoot)
{
	namespace fs = std::filesystem;

	fs::path    normalized  = fs::absolute(projectRoot).lexically_normal();
	std::string projectName = normalized.filename().string();

	if (projectName.empty())
	{
		throw std::runtime_error("Could not determine project name from path: " + projectRoot.string());
	}

	return projectName;
}

void initProjectAt(const std::filesystem::path &projectRoot, const std::string &projectName)
{
	namespace fs = std::filesystem;

	fs::create_directories(projectRoot);
	fs::create_directories(projectRoot / "Assets");

	writeFile(projectRoot / ".gitignore", makeGitignore());

	std::cout << "Created Anjean project: " << projectName << std::endl;
	std::cout << "Project path: " << fs::absolute(projectRoot).lexically_normal() << std::endl;
}

void initProject()
{
	namespace fs = std::filesystem;

	fs::path    projectRoot = fs::current_path();
	std::string projectName = getProjectNameFromPath(projectRoot);

	initProjectAt(projectRoot, projectName);
}

void initProject(const std::string &projectPathArg)
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
}        // namespace Anjean