#ifndef is_color_included

    #define is_color_included

    namespace color {

        using namespace std;

        const char *color_end = "\e[0m";

        const char *fg_black = "\e[030m";
        const char *fg_red = "\e[031m";
        const char *fg_green = "\e[032m";
        const char *fg_yellow = "\e[033m";
        const char *fg_blue = "\e[034m";
        const char *fg_purple = "\e[035m";
        const char *fg_cyan = "\e[036m";
        const char *fg_white = "\e[037m";

        const char *fg_black_bright = "\e[090m";
        const char *fg_red_bright = "\e[091m";
        const char *fg_green_bright = "\e[092m";
        const char *fg_yellow_bright = "\e[093m";
        const char *fg_blue_bright = "\e[094m";
        const char *fg_purple_bright = "\e[095m";
        const char *fg_cyan_bright = "\e[096m";
        const char *fg_white_bright = "\e[097m";

        const char *fg_black_bold = "\e[1;030m";
        const char *fg_red_bold = "\e[1;031m";
        const char *fg_green_bold = "\e[1;032m";
        const char *fg_yellow_bold = "\e[1;033m";
        const char *fg_blue_bold = "\e[1;034m";
        const char *fg_purple_bold = "\e[1;035m";
        const char *fg_cyan_bold = "\e[1;036m";
        const char *fg_white_bold = "\e[1;037m";

        const char *bg_black = "\e[040m";
        const char *bg_red = "\e[041m";
        const char *bg_green = "\e[042m";
        const char *bg_yellow = "\e[043m";
        const char *bg_blue = "\e[044m";
        const char *bg_purple = "\e[045m";
        const char *bg_cyan = "\e[046m";
        const char *bg_white = "\e[047m";

        const char *bg_black_bright = "\e[100m";
        const char *bg_red_bright = "\e[101m";
        const char *bg_green_bright = "\e[102m";
        const char *bg_yellow_bright = "\e[103m";
        const char *bg_blue_bright = "\e[104m";
        const char *bg_purple_bright = "\e[105m";
        const char *bg_cyan_bright = "\e[106m";
        const char *bg_white_bright = "\e[107m";

    }

#endif

