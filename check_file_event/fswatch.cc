/*************************************************************************
    > File Name: fswatch.cc
    > Author: hsz
    > Brief:
    > Created Time: 2024年02月22日 星期四 18时22分38秒
 ************************************************************************/

#include <iostream>
#include <string>
#include <exception>
#include <csignal>
#include <cstdlib>
#include <cmath>
#include <ctime>
#include <cerrno>
#include <vector>
#include <array>
#include <map>
#include "libfswatch/c/error.h"
#include "libfswatch/c/libfswatch.h"
#include "libfswatch/c++/path_utils.hpp"
#include "libfswatch/c++/event.hpp"
#include "libfswatch/c++/monitor.hpp"
#include "libfswatch/c++/monitor_factory.hpp"
#include "libfswatch/c++/libfswatch_exception.hpp"

static fsw::monitor *active_monitor = nullptr;

static void close_monitor()
{
    if (active_monitor)
        active_monitor->stop();
}

void close_handler(int signal)
{
  printf("Executing termination handler.\n");
  close_monitor();
}

static void register_signal_handlers()
{
    struct sigaction action{};
    action.sa_handler = close_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;

    if (sigaction(SIGTERM, &action, nullptr) == 0)
    {
        printf("SIGTERM handler registered.\n");
    }
    else
    {
        std::cerr << "SIGTERM handler registration failed." << std::endl;
    }

    if (sigaction(SIGABRT, &action, nullptr) == 0)
    {
        printf("SIGABRT handler registered.\n");
    }
    else
    {
        std::cerr << "SIGABRT handler registration failed." << std::endl;
    }

    if (sigaction(SIGINT, &action, nullptr) == 0)
    {
        printf(("SIGINT handler registered.\n"));
    }
    else
    {
        std::cerr << ("SIGINT handler registration failed") << std::endl;
    }
}

static void print_event_flags(const fsw::event& evt)
{
    const std::vector<fsw_event_flag>& flags = evt.get_flags();

    for (size_t i = 0; i < flags.size(); ++i)
    {
        switch (flags[i]) {
        case fsw_event_flag::NoOp:
            printf("NoOp ");
            break;
        case fsw_event_flag::PlatformSpecific:
            printf("PlatformSpecific ");
            break;
        case fsw_event_flag::Created:
            printf("Created ");
            break;
        case fsw_event_flag::Updated:
            printf("Updated ");
            break;
        case fsw_event_flag::Removed:
            printf("Removed ");
            break;
        case fsw_event_flag::Renamed:
            printf("Renamed ");
            break;
        case fsw_event_flag::OwnerModified:
            printf("OwnerModified ");
            break;
        case fsw_event_flag::AttributeModified:
            printf("AttributeModified ");
            break;
        case fsw_event_flag::MovedFrom:
            printf("MovedFrom ");
            break;
        case fsw_event_flag::MovedTo:
            printf("MovedTo ");
            break;
        case fsw_event_flag::IsFile:
            printf("IsFile ");
            break;
        case fsw_event_flag::IsDir:
            printf("IsDir ");
            break;
        case fsw_event_flag::IsSymLink:
            printf("IsSymLink ");
            break;
        case fsw_event_flag::Link:
            printf("Link ");
            break;
        case fsw_event_flag::Overflow:
            printf("Overflow ");
            break;

        default:
            break;
        }

        if (i != flags.size() - 1) std::cout << " ";
    }
}

static void print_event_timestamp(const fsw::event& evt)
{
    const time_t& evt_time = evt.get_time();

    static const uint32_t TIME_FORMAT_BUFF_SIZE = 128;
    std::array<char, TIME_FORMAT_BUFF_SIZE> time_format_buffer{};
    const struct tm *tm_time = localtime(&evt_time);

    std::string date =
    strftime(time_format_buffer.data(), time_format_buffer.size(), "%c", tm_time)
        ? std::string(time_format_buffer.data())
        : std::string("<date format error>");

    std::cout << date;
}

static void print_event_path(const fsw::event& evt)
{
    std::cout << evt.get_path();
}

static void process_events(const std::vector<fsw::event>& events, void *)
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    for (const fsw::event &evt : events)
    {
        print_event_timestamp(evt);
        printf("\t");
        print_event_path(evt);
        printf("\t");
        print_event_flags(evt);
        printf("\n");
    }
}

static void start_monitor(int argc, char **argv)
{
    std::vector<std::string> paths = { argv[1] };

    active_monitor = fsw::monitor_factory::create_monitor(fsw_monitor_type::system_default_monitor_type, paths, process_events);

    active_monitor->set_allow_overflow(true);
    active_monitor->set_latency(1.0);
    active_monitor->set_fire_idle_event(false);
    active_monitor->set_recursive(true);
    active_monitor->set_directory_only(false);

    std::vector<fsw_event_type_filter> event_filters = {
        { fsw_event_flag::Created },
        { fsw_event_flag::Updated },
        { fsw_event_flag::Removed },
        { fsw_event_flag::Renamed },
        { fsw_event_flag::OwnerModified },
        { fsw_event_flag::AttributeModified },
        { fsw_event_flag::MovedFrom },
        { fsw_event_flag::MovedTo },
        { fsw_event_flag::IsFile },
        { fsw_event_flag::IsDir },
        { fsw_event_flag::IsSymLink },
        { fsw_event_flag::Link },
        { fsw_event_flag::Overflow }
    };
    active_monitor->set_event_type_filters(event_filters);
    // active_monitor->set_filters(filters);
    active_monitor->set_follow_symlinks(true);
    active_monitor->set_watch_access(false);
    active_monitor->set_bubble_events(false);

    active_monitor->start();
}


int main(int argc, char **argv)
{
    if (argc != 2)
    {
        printf("Usage: %s /path\n", argv[0]);
        return -1;
    }

    try
    {
        register_signal_handlers();
        atexit(close_monitor);

        start_monitor(argc, argv);

        delete active_monitor;
        active_monitor = nullptr;
    }
    catch (const fsw::libfsw_exception& lex)
    {
        std::cerr << lex.what() << "\n";
        std::cerr << "Status code: " << lex.error_code() << "\n";

        return -1;
    }
    catch (const std::invalid_argument& ex)
    {
        std::cerr << ex.what() << "\n";

        return -1;
    }
    catch (const std::exception& conf)
    {
        std::cerr << ("An error occurred and the program will be terminated.\n");
        std::cerr << conf.what() << "\n";

        return -1;
    }
    catch (...)
    {
        std::cerr << ("An unknown error occurred and the program will be terminated.\n");

        return -1;
    }

    return 0;
}
