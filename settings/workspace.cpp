
#include "mmap.h"
#include "replaymanager.h"
#include "workspace.h"

#include <fstream>

using namespace nlohmann;

PGWorkspace::PGWorkspace() {
}

void PGWorkspace::LoadWorkspace(std::string filename) {
	this->filename = filename;
	lng result_size;
	PGFileError error;
	json j;
	char* ptr = (char*)panther::ReadFile(filename, result_size, error);
	if (!ptr) {
		goto default_workspace;
	}
	try {
		j = json::parse(ptr);
	} catch (...) {
		goto default_workspace;
	}

	this->settings = j;
	if (j.count("workspace_name") > 0) {
		this->workspace_name = j["workspace_name"].get<std::string>();
	}
	findtext.LoadWorkspace(j);
	if (j.count("windows") > 0 && j["windows"].is_array() && j["windows"].size() > 0) {
		for (int index = 0; index < j["windows"].size(); index++) {
			auto window = PGCreateWindow(this, std::vector<std::shared_ptr<TextFile>>());
			if (!window) {
				goto default_workspace;
			}
			PGLoadWorkspace(window, j["windows"][index]);
		}
		return;
	}
default_workspace:
	this->settings = j;
	this->workspace_name = "Default Workspace";
	auto window = PGCreateWindow(this, std::vector<std::shared_ptr<TextFile>>());
}

void PGWorkspace::WriteWorkspace() {
	if (PGGlobalReplayManager::running_replay) return;

	std::string errmsg;
	if (filename.size() == 0) return;

	json j = settings;
	findtext.WriteWorkspace(j);
	j["workspace_name"] = this->workspace_name;
	j["windows"] = nlohmann::json::array();
	int index = 0;
	for (auto it = windows.begin(); it != windows.end(); it++) {
		j["windows"][index] = nlohmann::json::object();
		PGWriteWorkspace(*it, j["windows"][index]);
		index++;
	}

	// we write the workspace to a temporary file first (workspace.json.tmp)
	std::string temp_filename = filename + ".tmp";

	std::ofstream out(temp_filename);
	if (!out) {
		return;
	}
	if (!(out << std::setw(4) << j)) {
		out.close();
		goto cleanup;
	}
	out.close();

	{
		// after writing to the temporary file, we move it over the old workspace file 
		auto ret = PGRenameFile(temp_filename, filename);
		if (ret == PGIOSuccess) {
			return;
		}
	}
cleanup:
	PGRemoveFile(temp_filename);
}