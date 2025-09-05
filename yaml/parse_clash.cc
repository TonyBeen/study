#include <iostream>
#include <yaml-cpp/yaml.h>
#include <string>

void parse_dns(const YAML::Node& dns) {
    std::cout << "\nDNS 配置:" << std::endl;
    std::cout << "  enable: " << dns["enable"].as<bool>() << std::endl;
    std::cout << "  ipv6: " << dns["ipv6"].as<bool>() << std::endl;

    std::cout << "  nameserver: ";
    for (const auto& ns : dns["nameserver"]) {
        std::cout << ns.as<std::string>() << " ";
    }
    std::cout << std::endl;

    std::cout << "  fallback: ";
    for (const auto& fb : dns["fallback"]) {
        std::cout << fb.as<std::string>() << " ";
    }
    std::cout << std::endl;

    if (dns["fallback-filter"]) {
        auto filter = dns["fallback-filter"];
        std::cout << "  fallback-filter:" << std::endl;
        std::cout << "    geoip: " << filter["geoip"].as<bool>() << std::endl;

        std::cout << "    ipcidr: ";
        for (const auto& ip : filter["ipcidr"]) {
            std::cout << ip.as<std::string>() << " ";
        }
        std::cout << std::endl;

        std::cout << "    domain: ";
        for (const auto& domain : filter["domain"]) {
            std::cout << domain.as<std::string>() << " ";
        }
        std::cout << std::endl;
    }
}

void parse_proxies(const YAML::Node& proxies) {
    std::cout << "\n代理节点(proxies):" << std::endl;
    for (const auto& proxy : proxies) {
        std::cout << "  name: " << proxy["name"].as<std::string>() << std::endl;
        std::cout << "    type: " << proxy["type"].as<std::string>() << std::endl;
        std::cout << "    server: " << proxy["server"].as<std::string>() << std::endl;
        std::cout << "    port: " << proxy["port"].as<int>() << std::endl;
        std::cout << "    password: " << proxy["password"].as<std::string>() << std::endl;
        std::cout << "    alpn: ";
        for (const auto& alpn : proxy["alpn"])
            std::cout << alpn.as<std::string>() << " ";
        std::cout << std::endl;
        std::cout << "    skip-cert-verify: " << proxy["skip-cert-verify"].as<bool>() << std::endl;
    }
}

void parse_proxy_groups(const YAML::Node& groups) {
    std::cout << "\n代理组(proxy-groups):" << std::endl;
    for (const auto& group : groups) {
        std::cout << "  name: " << group["name"].as<std::string>() << std::endl;
        std::cout << "    type: " << group["type"].as<std::string>() << std::endl;
        if (group["url"])
            std::cout << "    url: " << group["url"].as<std::string>() << std::endl;
        if (group["interval"])
            std::cout << "    interval: " << group["interval"].as<int>() << std::endl;
        std::cout << "    proxies: ";
        for (const auto& proxy : group["proxies"])
            std::cout << proxy.as<std::string>() << " ";
        std::cout << std::endl;
    }
}

void parse_rules(const YAML::Node& rules) {
    std::cout << "\n分流规则(rules):" << std::endl;
    for (const auto& rule : rules) {
        std::cout << "  " << rule.as<std::string>() << std::endl;
    }
}

int main() {
    YAML::Node config = YAML::LoadFile("clash.yaml");

    // 基本标量
    std::cout << "port: " << config["port"].as<int>() << std::endl;
    std::cout << "socks-port: " << config["socks-port"].as<int>() << std::endl;
    std::cout << "allow-lan: " << config["allow-lan"].as<bool>() << std::endl;
    std::cout << "mode: " << config["mode"].as<std::string>() << std::endl;
    std::cout << "log-level: " << config["log-level"].as<std::string>() << std::endl;
    std::cout << "external-controller: " << config["external-controller"].as<std::string>() << std::endl;
    std::cout << "secret: " << config["secret"].as<std::string>() << std::endl;

    // DNS
    YAML::Node node = config["dns"];
    if (node.IsDefined())
        parse_dns(node);

    // 代理
    node = config["proxies"];
    if (node.IsDefined())
        parse_proxies(node);

    // 代理组
    node = config["proxy-groups"];
    if (node.IsDefined())
        parse_proxy_groups(node);

    // 分流规则
    node = config["rules"];
    if (node.IsDefined())
        parse_rules(node);

    return 0;
}