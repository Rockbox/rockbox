#include "plugin.h"

struct gob_header {
    char magic[4];
    long MASTERX;
};

enum plugin_status plugin_start(const void* parameter)
{
    int gob_file = rb->open(parameter, O_RDONLY);

    struct gob_header current_header;
    rb->read(gob_file, &current_header, sizeof(struct gob_header));

    //I really should do magic checking here but i dont want to right now

    rb->lseek(gob_file, current_header.MASTERX, SEEK_SET);
    long mastern;
    rb->read(gob_file, &mastern, sizeof(long));
    rb->splashf(HZ, "the number of files is %d", (int)mastern);

    char list_array[mastern][13];

    //fill the list array
    int i;
    int ofs = sizeof(long)*3;
    for(i = 0; i != mastern; i++) {
        rb->lseek(gob_file, current_header.MASTERX + ofs, SEEK_SET);
        char name[13];
        rb->read(gob_file, &name, 13);
        rb->snprintf(list_array[i], 13, "%s", name);
        ofs = ofs + ((sizeof(long)*2)+13);
    }

    ////////////////////////////
    //end of good, tested code//
    ////////////////////////////



    //here are the list functions and declarations

    struct gui_synclist list;

    const char* list_get_name_cb(int item_number, void* data, char* buffer, size_t buffer_len)
    {
        rb->snprintf(buffer, 13, "%s", list_array[item_number]);
        return buffer;
    }

    rb->gui_synclist_init(&list,&list_get_name_cb,0, false, 1, NULL);
    rb->gui_synclist_set_icon_callback(&list,NULL);
    rb->gui_synclist_set_nb_items(&list,mastern);
    rb->gui_synclist_limit_scroll(&list,true);
    rb->gui_synclist_select_item(&list, 0);
    rb->gui_synclist_draw(&list);

    ///////////////////////////
    //and here comes the list//
    ///////////////////////////

    rb->lcd_clear_display();

    bool exit = false;
    int button = 0;

    while(!exit)
    {
        rb->gui_synclist_draw(&list);
        button = rb->get_action(CONTEXT_LIST,TIMEOUT_BLOCK);
        if (rb->gui_synclist_do_button(&list,&button,LIST_WRAP_UNLESS_HELD))
            continue;
        switch (button)
        {
            case ACTION_STD_CANCEL:
                exit = true;
                break;
        }
    }
    /////////////////
    //code to close//
    /////////////////

    rb->close(gob_file);

    return PLUGIN_OK;

}
