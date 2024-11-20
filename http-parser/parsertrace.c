/* Copyright Joyent, Inc. and other Node contributors.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/* Dump what the parser finds to stdout as it happen */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "http_parser.h"

int on_message_begin(http_parser *_)
{
    (void)_;
    printf("\n***MESSAGE BEGIN***\n\n");
    return 0;
}

int on_headers_complete(http_parser *_)
{
    (void)_;
    printf("\n***HEADERS COMPLETE***\n\n");
    return 0;
}

int on_message_complete(http_parser *_)
{
    (void)_;
    printf("\n***MESSAGE COMPLETE***\n\n");
    return 0;
}

int on_url(http_parser *_, const char *at, size_t length)
{
    (void)_;
    printf("Url: %.*s\n", (int)length, at);
    return 0;
}

int on_status(http_parser* _, const char* at, size_t length) {
    (void)_;
    printf("Status: %.*s\n", (int)length, at);
    return 0;
}

int on_header_field(http_parser *_, const char *at, size_t length)
{
    (void)_;
    printf("Header field: %.*s\n", (int)length, at);
    return 0;
}

int on_header_value(http_parser *_, const char *at, size_t length)
{
    (void)_;
    printf("Header value: \t%.*s\n", (int)length, at);
    return 0;
}

int on_body(http_parser *_, const char *at, size_t length)
{
    (void)_;
    printf("Body: '%.*s'\n", (int)length, at);
    return 0;
}

int on_chunk_header(http_parser* parser)
{
    // NOTE trunk的内容会通过on_body回调
    printf("on_chunk_header: %lu\n", parser->content_length);
    return 0;
}

int on_chunk_complete(http_parser* parser)
{
    printf("Chunk complete\n");
    return 0;
}

void usage(const char *name)
{
    fprintf(stderr,
            "Usage: %s $type $filename\n"
            "  type: -x, where x is one of {r,b,q}\n"
            "  parses file as a Response, reQuest, or Both\n",
            name);
    exit(EXIT_FAILURE);
}

int main()
{
    const char *getRequest = 
        "GET /templets/new/script/jquery?user=1&password=123#fragment HTTP/1.1\r\n"
        "Host: c.biancheng.net\r\n"
        "Content-Length: 0\r\n"
        "Proxy-Connection: keep-alive\r\n"
        "Pragma: no-cache\r\n"
        "Cache-Control: no-cache\r\n"
        "Accept: text/javascript, application/javascript, application/ecmascript, application/x-ecmascript, */*; q=0.01\r\n"
        "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/97.0.4692.71 Safari/537.36\r\n"
        "X-Requested-With: XMLHttpRequest\r\n"
        "Referer: http://c.biancheng.net/view/7918.html\r\n"
        "Accept-Encoding: gzip, deflate\r\n"
        "Accept-Language: zh-CN,zh;q=0.9\r\n\r\n";
    size_t getRequestLen = strlen(getRequest);

    const char *postRequest = 
        "POST /templets/new/script/jquery#fragment HTTP/1.1\r\n"
        "Host: c.biancheng.net\r\n"
        "Content-Length: 19\r\n"
        "Proxy-Connection: keep-alive\r\n"
        "Pragma: no-cache\r\n"
        "Cache-Control: no-cache\r\n"
        "Accept: text/javascript, application/javascript, application/ecmascript, application/x-ecmascript, */*; q=0.01\r\n"
        "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/97.0.4692.71 Safari/537.36\r\n"
        "X-Requested-With: XMLHttpRequest\r\n"
        "Referer: http://c.biancheng.net/view/7918.html\r\n"
        "Accept-Encoding: gzip, deflate\r\n"
        "Accept-Language: zh-CN,zh;q=0.9\r\n\r\n"
        "user=1&password=123";
    size_t postRequestLen = strlen(postRequest);

    const char *chunkResponse =
        "HTTP/1.1 200 OK\r\n"
        "Transfer-Encoding: chunked\r\n"
        "\r\n"
        "4\r\n"
        "Wiki\r\n"
        "5\r\n"
        "pedia\r\n"
        "0\r\n"
        "\r\n";
    size_t chunkResponseLen = strlen(chunkResponse);

    http_parser_settings settings;
    memset(&settings, 0, sizeof(settings));
    settings.on_message_begin = on_message_begin;
    settings.on_url = on_url;
    settings.on_status = on_status;
    settings.on_header_field = on_header_field;
    settings.on_header_value = on_header_value;
    settings.on_headers_complete = on_headers_complete;
    settings.on_body = on_body;
    settings.on_message_complete = on_message_complete;
    settings.on_chunk_header = on_chunk_header;
    settings.on_chunk_complete = on_chunk_complete;

    srand(time(NULL));
    {
        // http-parser 支持分段解析, 如果body过长时可以采用此中方式
        // 然而会出现同一个接口调用两次情况, 需要使用append方式进行处理
        /**
            Url: /templet
            Url: s/new/script/jquery?user=1&password=123#fragment
            Header field: Host
            Header value:   c.biancheng.net
            Header field: Content-Length
            Header value:   0
            Header field: Proxy-Connection
            Header value:   keep-alive
            Header field: Pragma
            Header value:   no-cache
            Header field: Cache-Control
            Header value:   no-cache
            Header field: Accept
            Header value:   text/javascript, application/javascript, application/ecmascript, application/x-ecmascript, *\/*; q=0.01
            Header field: User-Agent
            Header value:   Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/97.0.4692.71 Safari/537.36
            Header field: X-Requested-With
            Header value:   XMLHttpRequest
            Header field: Referer
            Header value:   http://c.biancheng.net/view/7918.html
            Header field: Accept-Encoding
            Header value:   gzip, deflate
            Header field: Accept-Language
            Header value:   zh-CN,zh;q=0.9
         */
        int32_t firstSize = rand() % (int32_t)getRequestLen + 1;
        http_parser parser;
        http_parser_init(&parser, HTTP_REQUEST);
        size_t nparsed = http_parser_execute(&parser, &settings, getRequest, firstSize);
        if (nparsed != firstSize)
        {
            fprintf(stderr, "Error: %s (%s)\n",
                    http_errno_description(HTTP_PARSER_ERRNO(&parser)),
                    http_errno_name(HTTP_PARSER_ERRNO(&parser)));
        }

        nparsed = http_parser_execute(&parser, &settings, getRequest + firstSize, getRequestLen - firstSize);
        if (nparsed != (getRequestLen - firstSize))
        {
            fprintf(stderr, "Error: %s (%s)\n",
                    http_errno_description(HTTP_PARSER_ERRNO(&parser)),
                    http_errno_name(HTTP_PARSER_ERRNO(&parser)));
        }
    }

    {
        http_parser parser;
        http_parser_init(&parser, HTTP_REQUEST);
        size_t nparsed = http_parser_execute(&parser, &settings, postRequest, postRequestLen);
        if (nparsed != getRequestLen)
        {
            fprintf(stderr, "Error: %s (%s)\n",
                    http_errno_description(HTTP_PARSER_ERRNO(&parser)),
                    http_errno_name(HTTP_PARSER_ERRNO(&parser)));
        }
    }

    {
        http_parser parser;
        http_parser_init(&parser, HTTP_RESPONSE);
        size_t nparsed = http_parser_execute(&parser, &settings, chunkResponse, chunkResponseLen);
        if (nparsed != chunkResponseLen)
        {
            fprintf(stderr, "Error: %s (%s)\n",
                    http_errno_description(HTTP_PARSER_ERRNO(&parser)),
                    http_errno_name(HTTP_PARSER_ERRNO(&parser)));
        }
    }

    return EXIT_SUCCESS;
}
