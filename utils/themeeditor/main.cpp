namespace wps
{
    extern "C"
    {
#include "skin_parser.h"
#include "skin_debug.h"
    }
}

#include <cstdlib>
#include <cstdio>

#include <QtGui/QApplication>
#include <QFileSystemModel>
#include <QTreeView>

int main(int argc, char* argv[])
{

    char* doc = "%Vd(U)\n\n%?bl(test,3,5,2,1)<param2|param3>";

    struct wps::skin_element* test = wps::skin_parse(doc);

    wps::skin_debug_tree(test);

    wps::skin_free_tree(test);


    return 0;
}

