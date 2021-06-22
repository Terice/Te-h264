#include "terror.h"
#include <stdlib.h>
#include <iostream>
#include <lua5.1/lua.hpp>

terror terr;

terror::terror()
{
    tconf("conf.lua");
}

terror::~terror()
{

}

void terror::error(std::string str)
{
    std::cout << ">>terr: " << str << std::endl;
    exit(-1);
}

void terror::tconf(std::string conf)
{
    lua_State *L = lua_open();
    luaL_openlibs(L);
    int conf_length = sizeof(DebugControl)/sizeof(char);
    int paus_length = sizeof(PausePoint)/sizeof(char);
    //先load一次lua文件，然后执行一次匿名函数（也就是这个文件）
    if(luaL_loadfile(L, conf.c_str()) || lua_pcall(L,0,conf_length+paus_length,0))
    {
        std::cout << "lua file open error " << std::endl;
        lua_close(L);
        exit(-1);
    }
    // 把结构体的首地址转换成指针
    char *parameters = (char*)&de;
    // 然后用lua的栈按按顺序返回这些控制符
    int i;
    for (i = 0; i < conf_length; i++)
    {
        //因为栈是从1开始的，所以i+1
        parameters[i] = (char)lua_tonumber(L, i+1);
    }
    parameters = (char*)&po;
    for(int j = 0; j < paus_length; j++)
    {
        parameters[j] = (char)lua_tonumber(L, ++i);
    }

    lua_close(L);
}
void terror::temsg(char *s)
{
    std::cout << s << std::endl;
}
void terror::temsg(std::string s)
{
    std::cout << s << std::endl;
}
void terror::texit(int code)
{
    std::cout << ">> terr: exit at:[" << code << "]" <<std::endl; 
    exit(code);
}
