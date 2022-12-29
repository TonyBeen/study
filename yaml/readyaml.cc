#include <yaml-cpp/yaml.h>
#include <fstream>
#include <string>
#include <iostream>
#include <string.h>
#include <vector>

using namespace std;

YAML::Node *LoadYamlFile(const char *path = "./config.yml")
{
    if (path == nullptr) {
        return nullptr;
    }
    YAML::Node config = YAML::LoadFile(path);
    cout << config << endl;
    return new YAML::Node(config);
}

const char *CreateYaml()
{
    // ma
    const char *path = "./config.yml";
    return path;
}

void read_write_yaml(YAML::Node *node)
{
    
}


int main()
{
    YAML::Node *config = LoadYamlFile(CreateYaml());
    if (config == nullptr) {
        perror("LoadYamlFile return null");
        return -1;
    }
    cout << (*config)["log"]["level"].as<std::string>() << endl;
    (*config)["log"]["level"] = "info";

    fstream fout("./config.yml");
    fout << *config;
    return 0;
}