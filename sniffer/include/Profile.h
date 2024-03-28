#ifndef STC_PROFILE_H
#define STC_PROFILE_H

#include <string>
#include <map>
#include <vector>

class ProfileType{
public:
    static std::map<uint32_t, const char*> typeStr;

    ProfileType(uint32_t type, std::string name, uint32_t len);

    std::string getName();
    uint32_t getType();
    uint32_t getLen();

private:

    std::string _name;
    uint32_t _type;
    uint32_t _len;
};

class Profile {
public:
    Profile();
    ~Profile();

    static Profile* load(const std::string& path);
    static void save(const std::string& name, Profile* profile);

    std::map<uint32_t , std::vector<ProfileType*>*> types;
private:

};


#endif //STC_PROFILE_H
