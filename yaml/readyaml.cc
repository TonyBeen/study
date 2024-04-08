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

    cout << (*config)["log"]["sync"].as<bool>() << endl;

    {
        std::cout << "*******************************************************\n";
        auto node = (*config)["log"]["test"];
        cout << "test type = " << node.Type() << ", style = " << node.Style() << endl;
        if (node.IsSequence()) {
            for (int i = 0; i < node.size(); ++i) {
                const YAML::Node &temp = node[i];
                cout << "type = " << temp.Type() << endl;
                cout << node[i] << endl;
                if (temp.IsSequence()) {
                    for (int i = 0; i < temp.size(); ++i) {
                        cout << temp[i].as<int>() << "\t";
                    }
                }
                cout << endl;
            }
        }
    }

    {
        std::cout << "*******************************************************\n";
        auto node = (*config)["array"];
        cout << "array type = " << node.Type() << ", style = " << node.Style() << endl;
        if (node.IsSequence()) {
            for (int i = 0; i < node.size(); ++i) {
                cout << node[i].Scalar() << ", tag = " << node[i].Tag() << endl;
            }
        }
    }

    {
        std::cout << "*******************************************************\n";
        const YAML::Node &node = (*config)["containers"];
        cout << "containers type = " << node.Type() << ", style = " << node.Style() << endl;
        if (node.IsSequence()) {
            for (int i = 0; i < node.size(); ++i) {
                const YAML::Node &temp = node[i];
                cout << "type = " << temp.Type() << endl;
                if (temp.IsMap()) {
                    for (auto it = temp.begin(); it != temp.end(); ++it) {
                        cout << "[" << it->first.Scalar() << ", " << it->second.Scalar() << "]\n";
                    }
                }
            }
        }
    }

    return 0;
}