#include "Profile.h"
#include "Version.h"
#include <utility>
#include <sys/stat.h>
#include <cstring>
#include <climits>
#include <sstream>

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

Profile *Profile::load(const std::string& path) {
    FILE *file = fopen(path.c_str(), "r");
    if (file == nullptr) {
        printf("Error opening file %s: %s\n",path.c_str(), strerror(errno));
        return nullptr;
    }
    char line[256];
    memset(line, 0, 256);
    // empty file
    if(fgets(line, sizeof(line), file) == NULL){
        fclose(file);
        return nullptr;
    }
    // Check Version
    if(stod(std::string(line)) < MIN_COMP_VERSION){
        printf("Error version(%f) is older than last supported version(%s) in %s\n", atof(line), std::to_string(MIN_COMP_VERSION).c_str(), path.c_str());
        fclose(file);
        return nullptr;
    }
    memset(line, 0, 256);

    Profile* profile = new Profile();
    char* token;
    while (fgets(line, sizeof(line), file) != NULL) {
        token = strtok(line, ";");
        if(token == NULL)
            continue;
        uint32_t id = atoi(token);
        if(profile->types.count(id) == 0)
            profile->types[id] = new std::vector<ProfileType*>;

        token = strtok(NULL, ";");
        if(token == NULL)
            continue;
        uint32_t type = atoi(token);

        token = strtok(NULL, ";");
        if(token == NULL)
            continue;
        uint32_t len = atoi(token);

        token = strtok(NULL, ";");
        if(token == NULL)
            continue;

        profile->types[id]->push_back(new ProfileType(type, std::string(token, strlen(token) -1), len));
        memset(line, 0, 256);
    }

    fclose(file);
    return profile;
}

void Profile::save(const std::string& name, Profile *profile) {
    struct stat st = {0};

    const char *home_dir = getenv("HOME");
    if (home_dir == NULL) {
        fprintf(stderr, "Error: HOME environment variable not set.\n");
        return;
    }
    char path[PATH_MAX];
    snprintf(path, sizeof(path), "%s/%s", home_dir, "STCSniffer");

    if (stat(path, &st) == -1) {
        if(mkdir(path, 0770))
            printf("Error creating dir %s: %s\n", path, strerror(errno));
    }
    snprintf(path, sizeof(path), "%s/%s", home_dir, "STCSniffer/Profiles");
    if (stat(path, &st) == -1) {
        if(mkdir(path, 0770))
            printf("Error creating dir %s: %s\n", path, strerror(errno));
    }

    std::stringstream ss;
    ss << path << "/" << name << ".stc_profile";
    FILE *file = fopen(ss.str().c_str(), "w");
    if (file == nullptr) {
        printf("Error opening file %s: %s\n", ss.str().c_str(), strerror(errno));
        return;
    }

    fprintf(file, "%s\n", VERSION);
    for (auto pair : profile->types) {
        uint32_t id = pair.first;
            for (auto type : *pair.second) {
                fprintf(file, "%d;%d;%d;%s\n", id, type->getType(), type->getLen(), type->getName().c_str());
            }
    }

    fclose(file);
}

