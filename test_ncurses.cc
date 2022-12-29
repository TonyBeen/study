/*************************************************************************
    > File Name: test_ncurses.cc
    > Author: hsz
    > Brief:
    > Created Time: Tue 20 Sep 2022 02:16:36 PM CST
 ************************************************************************/

#include <locale.h>

#include <menu.h>

#include <stdio.h>

#include <ctype.h>

//定义菜单项

static const char *menus[] = {

    "1-1", "1-2", "1-3", "2-1", "2-2", "2-3"

};

#define CITEM sizeof(menus) / sizeof(menus[0]) //菜单项数

ITEM *items[CITEM];

int main(int argc, char *argv[])
{

    int i;

    int ch;

    int mrows, mcols;

    WINDOW *win, *subwin;

    MENU *mymenu;

    //初始化屏幕

    initscr();

    //不用等待回车键

    cbreak();

    //不回显

    noecho();

    //可以处理功能键

    keypad(stdscr, TRUE);

    //建立菜单项

    for (i = 0; i < CITEM; i++)
    {

        items[i] = new_item(menus[i], menus[i]); //第二个参数为菜单项的描述
    }

    //建立菜单

    mymenu = new_menu(items);

    set_menu_format(mymenu, CITEM, 1); //设置CITEM行1列的菜单

    set_menu_mark(mymenu, ">"); //菜单选中的MARK

    //获得菜单的行列数

    scale_menu(mymenu, &mrows, &mcols);

    //建立窗口和子窗口

    win = newwin(mrows + 2, mcols + 2, 3, 30);

    keypad(win, TRUE);

    box(win, 0, 0);

    subwin = derwin(win, 0, 0, 1, 1);

    //设置菜单的窗口

    set_menu_sub(mymenu, subwin);

    //在子窗口上放置菜单

    post_menu(mymenu);

    refresh();

    wrefresh(win);

    //获得输入，并移动选择到相应的菜单项

    while (toupper(ch = wgetch(win)) != '\n')
    {
        if (ch == KEY_DOWN)
            menu_driver(mymenu, REQ_DOWN_ITEM); //移动菜单选择
        else if (ch == KEY_RIGHT)
            menu_driver(mymenu, REQ_RIGHT_ITEM);
        else if (ch == KEY_UP)
            menu_driver(mymenu, REQ_UP_ITEM);
        else if (ch == KEY_LEFT)
            menu_driver(mymenu, REQ_LEFT_ITEM);
    }

    //输出当前项

    mvprintw(LINES - 2, 0, "you select the item :%s\n", item_name(current_item(mymenu)));

    refresh();

    unpost_menu(mymenu);

    getch();

    //释放内存

    free_menu(mymenu);

    for (i = 0; i < CITEM; i++)
        free_item(items[i]);

    endwin();

    return 1;
}
