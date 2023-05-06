#include <yaml-cpp/yaml.h>
#include <fstream>
#include <string>
#include <iostream>
#include <string.h>
#include <vector>

using namespace std;

void LoadYamlFile()
{
    YAML::Node config = YAML::LoadFile("./config.yaml");
    cout << config << endl;

    config["log"]["leval"] = "I";
    std::string fmt = config["log"]["dateformat"].as<std::string>();
    cout << fmt << endl;
    char buf[32] = {0};
    sprintf(buf, fmt.c_str(), 1, 1, 1, 1, 1, 1, 1);
    cout << "buf = " << buf << "\n";
    ofstream fout("./config.yaml");
    fout << config;
}

void ConstructYaml_seq()
{
    YAML::Emitter out;
    out << YAML::BeginSeq;
    out << "eggs bread milk";
    out << YAML::EndSeq;

    ofstream fout("./config.yaml");
    fout << out.c_str();
    std::cout << "内容\n"
              << out.c_str() << std::endl
              << out.size() << "\nstrlen = " << strlen(out.c_str()) << std::endl;
    fout.close();
}

void ConstructYaml_map()
{
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "name";
    out << YAML::Value << "Ryan Braun";
    out << YAML::Key << "position";
    out << YAML::Value << "LF";

    out << YAML::Key << "seq";
    out << YAML::Value << YAML::BeginSeq << "a" << "b" << "c" << YAML::EndSeq;
    out << YAML::EndMap;

    ofstream fout("./config.yaml");
    fout << out.c_str() << endl;
}

void ConstructYaml_doc()
{
    YAML::Emitter out;
    out << YAML::BeginDoc;
    out << YAML::BeginMap;
    out << YAML::Key << "seq";
    out << YAML::Value << YAML::BeginSeq << "a" << "b" << "c" << YAML::EndSeq;
    out << YAML::EndMap;
    out << YAML::EndDoc;

    ofstream fout("./config.yaml");
    fout << out.c_str() << endl;
}

void map_seq()
{
    YAML::Emitter out;
    out << YAML::BeginMap;
    
    // student: [Sasha, Malia]
    out << YAML::Key << "student";
    out << YAML::Value << YAML::Flow << YAML::BeginSeq << "Sasha" << "Malia" << YAML::EndSeq;

    // env:
    // - name: NKCLD_HOME
    //   value: /nkcld/nkcldrun
    // - name: NKCLD3PATH
    //   value: /nkcld/nkcld3
    out << YAML::Key << "env";
    out << YAML::Value;
    out << YAML::BeginSeq;
    out << YAML::BeginMap
        << YAML::Key << "name" << YAML::Value << "NKCLD_HOME"
        << YAML::Key << "value" << YAML::Value << "/nkcld/nkcldrun"
        << YAML::EndMap;
    out << YAML::BeginMap
        << YAML::Key << "name" << YAML::Value << "NKCLD3PATH"
        << YAML::Key << "value" << YAML::Value << "/nkcld/nkcld3"
        << YAML::EndMap;
    out << YAML::EndSeq;

    out << YAML::EndMap;

    YAML::Emitter emit;

    ofstream fout("./config.yaml", ios::in | ios::out | ios::app);
    std::cout << out.c_str() << std::endl;
    fout << out.c_str() << endl;
}

void create_array()
{
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "array";
    out << YAML::Value;
    out << YAML::BeginSeq << 2 << 3 << 5 << 7 << 11 << YAML::EndSeq;
    out << YAML::EndMap;

    fstream fout("./config.yaml", ios::in | ios::out | ios::app);
    fout << out.c_str() << endl;
}

void create_note()
{
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "method";
    out << YAML::Value << "post";
    out << YAML::Comment("should we change this method to get?");
    out << YAML::EndMap;

    fstream fout("./config.yaml", ios::in | ios::out | ios::app);
    fout << out.c_str() << endl;
}

void create_aliases_anchors()
{
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "alias";
    out << YAML::BeginSeq;
    out << YAML::Anchor("fred"); // 锚点
    out << YAML::BeginMap;
    out << YAML::Key << "name" << YAML::Value << "Fred";
    out << YAML::Key << "age" << YAML::Value << "42";
    out << YAML::EndMap;
    out << YAML::Alias("fred"); // 别名
    out << YAML::EndSeq;
    out << YAML::EndMap;

    fstream fout("./config.yaml", ios::in | ios::out | ios::app);
    fout << out.c_str() << endl;
}

struct Vec3
{
    int x;
    int y;
    int z;
};
YAML::Emitter &operator<<(YAML::Emitter &out, const Vec3 &v)
{
    out << YAML::BeginSeq;
    out << YAML::Flow;
    out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
    out << YAML::EndSeq;
    return out;
}

void test_operator()
{
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "test_operator";
    Vec3 vec;
    vec.x = 1;
    vec.y = 2;
    vec.z = 3;
    out << vec;
    out << YAML::EndMap;

    fstream fout("./config.yaml", ios::in | ios::out | ios::app);
    fout << out.c_str() << endl;
}

int main()
{
    // ConstructYaml_seq();
    // ConstructYaml_map();
    // ConstructYaml_doc();
    map_seq();
    create_array();
    // create_note();
    // create_aliases_anchors();
    return 0;
}