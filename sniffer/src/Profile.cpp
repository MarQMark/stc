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

ProfileType::ProfileType(uint32_t type, std::string name, uint32_t len)
    : _type(type), _name(std::move(name)) {

    switch (type) {
        case 0:
            _len = 1;
            break;
        case 1:
            _len = 2;
            break;
        case 2:
            _len = 4;
            break;
        case 3:
            _len = 8;
            break;
        case 4:
            _len = 1;
            break;
        case 5:
            _len = 2;
            break;
        case 6:
            _len = 4;
            break;
        case 7:
            _len = 8;
            break;
        case 8:
            _len = 4;
            break;
        case 9:
            _len = 8;
            break;
        case 10:
            _len = 1;
            break;
        case 11:
        case 12:
            _len = len;
            break;
    }
}

std::string ProfileType::getName() {
    return _name;
}

uint32_t ProfileType::getType() {
    return _type;
}

uint32_t ProfileType::getLen() {
    return _len;
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

