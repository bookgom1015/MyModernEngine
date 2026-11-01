#pragma once

struct TaskInfo {
	ETask::Type	Type;
	DWORD_PTR	Param_0;
	DWORD_PTR	Param_1;
	DWORD_PTR	Param_2;
};

struct ScriptInfo {
	std::string Name;
	ScriptFactory Factory;
};

struct Primitive {
	UINT BaseVertexLocation;
	UINT VertexCount;

	UINT StartIndexLocation;
	UINT IndexCount;
};
