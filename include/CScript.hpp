#pragma once

#include "Component.hpp"

struct Property {
    std::string Name;
    size_t Offset;

    enum EType {
        Bool,
        Int,
        Float,
        Vec2,
        Vec3,
        Object
    } Type;
};

class CScript : public Component {
public:
	CScript();
	virtual ~CScript();

public:
    virtual bool Final() final;

public:
    virtual Component* Clone() = 0;

    virtual bool SaveToLevelFile(FILE* const pFile) = 0;
    virtual bool LoadFromLevelFile(FILE* const pFile) = 0;

    virtual Hash GetID() const = 0;

protected:
    void Destroy();

	void AddProperty(const std::string& name, size_t offset, Property::EType type);

private:
	std::vector<Property> mProperties;
};