/*************************************************************************
    > File Name: utf8_test.cpp
    > Author: hsz
    > Brief:
    > Created Time: Fri 06 Jan 2023 03:23:58 PM CST
 ************************************************************************/

#include "tinyxml2.h"
#include <iostream>

using namespace std;
using namespace tinyxml2;

int gPass = 0;
int gFail = 0;

bool XMLTest(const char *testString, const char *expected, const char *found, bool echo = true, bool extraNL = false)
{
    bool pass;
    if (!expected && !found)
        pass = true;
    else if (!expected || !found)
        pass = false;
    else
        pass = !strcmp(expected, found);
    if (pass)
        printf("[pass]");
    else
        printf("[fail]");

    if (!echo)
    {
        printf(" %s\n", testString);
    }
    else
    {
        if (extraNL)
        {
            printf(" %s\n", testString);
            printf("%s\n", expected);
            printf("%s\n", found);
        }
        else
        {
            printf(" %s [%s][%s]\n", testString, expected, found);
        }
    }

    if (pass)
        ++gPass;
    else
        ++gFail;
    return pass;
}

bool XMLTest(const char *testString, XMLError expected, XMLError found, bool echo = true, bool extraNL = false)
{
    return XMLTest(testString, XMLDocument::ErrorIDToName(expected), XMLDocument::ErrorIDToName(found), echo, extraNL);
}

bool XMLTest(const char *testString, bool expected, bool found, bool echo = true, bool extraNL = false)
{
    return XMLTest(testString, expected ? "true" : "false", found ? "true" : "false", echo, extraNL);
}

template <class T>
bool XMLTest(const char *testString, T expected, T found, bool echo = true)
{
    bool pass = (expected == found);
    if (pass)
        printf("[pass]");
    else
        printf("[fail]");

    if (!echo)
        printf(" %s\n", testString);
    else
    {
        char expectedAsString[64];
        XMLUtil::ToStr(expected, expectedAsString, sizeof(expectedAsString));

        char foundAsString[64];
        XMLUtil::ToStr(found, foundAsString, sizeof(foundAsString));

        printf(" %s [%s][%s]\n", testString, expectedAsString, foundAsString);
    }

    if (pass)
        ++gPass;
    else
        ++gFail;
    return pass;
}

void NullLineEndings(char *p)
{
    while (p && *p)
    {
        if (*p == '\n' || *p == '\r')
        {
            *p = 0;
            return;
        }
        ++p;
    }
}

int main(int argc, char **argv)
{
    XMLDocument doc;
    doc.LoadFile("resources/utf8test.xml");
    XMLTest("Load utf8test.xml", false, doc.Error());

    // Get the attribute "value" from the "Russian" element and check it.
    XMLElement *element = doc.FirstChildElement("document")->FirstChildElement("Russian");
    const unsigned char correctValue[] = {0xd1U, 0x86U, 0xd0U, 0xb5U, 0xd0U, 0xbdU, 0xd0U, 0xbdU,
                                          0xd0U, 0xbeU, 0xd1U, 0x81U, 0xd1U, 0x82U, 0xd1U, 0x8cU, 0};

    XMLTest("UTF-8: Russian value.", (const char *)correctValue, element->Attribute("value"));

    const unsigned char russianElementName[] = {0xd0U, 0xa0U, 0xd1U, 0x83U,
                                                0xd1U, 0x81U, 0xd1U, 0x81U,
                                                0xd0U, 0xbaU, 0xd0U, 0xb8U,
                                                0xd0U, 0xb9U, 0};
    const char russianText[] = "<\xD0\xB8\xD0\xBC\xD0\xB5\xD0\xB5\xD1\x82>";

    XMLText *text = doc.FirstChildElement("document")->FirstChildElement((const char *)russianElementName)->FirstChild()->ToText();
    XMLTest("UTF-8: Browsing russian element name.", russianText, text->Value());

    // Now try for a round trip.
    doc.SaveFile("resources/out/utf8testout.xml");
    XMLTest("UTF-8: Save testout.xml", false, doc.Error());

    // Check the round trip.
    bool roundTripOkay = false;

    FILE *saved = fopen("resources/out/utf8testout.xml", "r");
    XMLTest("UTF-8: Open utf8testout.xml", true, saved != 0);

    FILE *verify = fopen("resources/utf8testverify.xml", "r");
    XMLTest("UTF-8: Open utf8testverify.xml", true, verify != 0);

    if (saved && verify)
    {
        roundTripOkay = true;
        char verifyBuf[256];
        while (fgets(verifyBuf, 256, verify))
        {
            char savedBuf[256];
            fgets(savedBuf, 256, saved);
            NullLineEndings(verifyBuf);
            NullLineEndings(savedBuf);

            if (strcmp(verifyBuf, savedBuf))
            {
                printf("verify:%s<\n", verifyBuf);
                printf("saved :%s<\n", savedBuf);
                roundTripOkay = false;
                break;
            }
        }
    }
    if (saved)
        fclose(saved);
    if (verify)
        fclose(verify);
    XMLTest("UTF-8: Verified multi-language round trip.", true, roundTripOkay);
    return 0;
}
