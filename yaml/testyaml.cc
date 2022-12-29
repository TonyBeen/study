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

/*     config["log"]["leval"] = "I";
    std::string fmt = config["log"]["dateformat"].as<std::string>();
    cout << fmt << endl;
    char buf[32] = {0};
    sprintf(buf, fmt.c_str(), 1, 1, 1, 1, 1, 1, 1);
    cout << "buf = " << buf << "\n";
    ofstream fout("./config.yaml");
    fout << config; */
}

void ConstructYaml_seq()
{
    YAML::Emitter out;
    out << YAML::BeginSeq;
    out << "eggs bread milk";
    out << YAML::EndSeq;

    ofstream fout("./config.yaml");
    fout << out.c_str();
    std::cout << "内容\n" << out.c_str() << std::endl
    << out.size() << "\nstrlen = "<< strlen(out.c_str()) << std::endl;
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
    out << YAML::EndMap;

    ofstream fout("./config.yaml");
    fout << out.c_str() << endl;
}

void map_seq()
{
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "name";
    out << YAML::Value << "Barack Obama";
    out << YAML::Key << "children";
    out << YAML::Value <<
        YAML::BeginSeq << "Sasha" << "Malia" << YAML::EndSeq;
    out << YAML::EndMap;

    ofstream fout("./config.yaml",  ios::in | ios::out | ios::app);
    fout << out.c_str() << endl;
}

void create_array()
{
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "array";
    out << YAML::Value;
    out << YAML::Flow;
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
    out << YAML::Anchor("fred");    // 锚点
    out << YAML::BeginMap;
    out << YAML::Key << "name" << YAML::Value << "Fred";
    out << YAML::Key << "age" << YAML::Value << "42";
    out << YAML::EndMap;
    out << YAML::Alias("fred");     // 别名
    out << YAML::EndSeq;
    out << YAML::EndMap;

    fstream fout("./config.yaml", ios::in | ios::out | ios::app);
    fout << out.c_str() << endl;
}

struct Vec3 { int x; int y; int z; };
YAML::Emitter& operator << (YAML::Emitter& out, const Vec3& v)
{
    out << YAML::BeginSeq;
	out << YAML::Flow;
	out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
    out << YAML:: EndSeq;
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
    LoadYamlFile();
    return 0;
}