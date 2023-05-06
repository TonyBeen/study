/*************************************************************************
    > File Name: xml_w.cpp
    > Author: hsz
    > Brief:
    > Created Time: Fri 06 Jan 2023 03:41:57 PM CST
 ************************************************************************/

#include "tinyxml2.h"
#include <iostream>
#include <string>

using namespace std;
using namespace tinyxml2;

void LoadXml(std::string prefix, XMLElement *ele)
{
    if (!ele) {
        return;
    }

    XMLElement *curr = ele->FirstChildElement();
    for (; curr; curr = curr->NextSiblingElement()) {
        printf("name: %s.%s\t\t\tvalue: %s\n", prefix.c_str(), curr->Name(), curr->GetText());
        if (curr->NoChildren() == false) {
            LoadXml(prefix.empty() ? curr->Name() : prefix + "." + curr->Name(), curr);
        }
    }
}

int main(int argc, char **argv)
{
    XMLDocument doc;
    doc.LoadFile("for_xml_w.xml");
    if (doc.ErrorID()) {
        return 1;
    }

    XMLElement *curr = doc.RootElement();
    for (; curr; curr = curr->NextSiblingElement()) {
        LoadXml(curr->Name(), curr);
    }
    return 0;
}
