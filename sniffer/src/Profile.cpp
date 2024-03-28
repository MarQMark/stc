#include "Profile.h"
#include <utility>

std::map<uint32_t, const char*> ProfileType::typeStr = std::map<uint32_t, const char*> {
        {0, "uint8"},
        {1, "uint16"},
        {2, "uint32"},
        {3, "uint64"},
        {4, "int8"},
        {5, "int16"},
        {6, "int32"},
        {7, "int64"},
        {8, "float"},
        {9, "double"},
        {10, "char"},
        {11, "string"},
        {12, "array"},
};

ProfileType::ProfileType(uint32_t id, uint32_t type, std::string name, uint32_t len)
    : _id(id), _type(type), _name(std::move(name)), _len(len) {

}


Profile::Profile() {

}

Profile::~Profile() {
    for (auto pair : types) {
        for (auto* type : *pair.second) {
            delete type;
        }
        // TODO: Fix
    }
}

Profile *Profile::load() {
    return new Profile;
}

void Profile::save(Profile *profile) {

}

