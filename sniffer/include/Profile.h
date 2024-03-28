#ifndef STC_PROFILE_H
#define STC_PROFILE_H

#include <string>
#include <map>
#include <vector>

class ProfileType{
public:
    static std::map<uint32_t, const char*> typeStr;

    ProfileType(uint32_t id, uint32_t type, std::string name, uint32_t len);

private:

    uint32_t _id;
    uint32_t _type;
    uint32_t _len;
    std::string _name;
};

class Profile {
public:
    Profile();
    ~Profile();

    static Profile* load();
    static void save(Profile* profile);

    std::map<uint32_t , std::vector<ProfileType*>*> types;
private:

};


#endif //STC_PROFILE_H
