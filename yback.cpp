#include <iostream>
#include <functional>
#include <map>
#include <fstream>
#include <sstream>
#include "header/getopt_class.h"
#include "header/misc.h"
#include "header/color.h"

using namespace std;

namespace prm {

    const vector<string> option_list = {
                                         "-s", "-show",
                                         "+f", "+file",
                                         "+tmp-directory",
                                         "+add",
                                         "+only-add",
                                         "+managed-file-list",
                                         "+b", "+bkrc",
                                         "-n", "-dry-run",
                                         "-h", "-help",
                                       };

    const string default_tmp_directory = "/tmp";

    const string default_managed_file_list_file = misc::word_expansion("~/.yback_managed_file_list")[0];

    const string lock_file = misc::word_expansion("~/.yback.lock")[0];

    void print_usage() {

        if (isatty(1)) {

            cout << "\
\e[;100mUsage\e[0m\n\
  yback [<option(s)>] [-- [<rsync option(s)>]]\n\
  \n\
\e[;100mOptions\e[0m\n\
  \e[94m--bkrc,-b <file>\e[0m              read <file> as a config file\n\
  \e[94m--file,-f <file>\e[0m              copy <file> to `/tmp` before any other operations\n\
  \e[94m--tmp-directory <dir>\e[0m         use <dir> instead of `/tmp` as the tmp directory\n\
  \e[94m--add <file>\e[0m                  add <file> to the managed file list before any other operations\n\
  \e[94m--only-add <file>\e[0m             add <file> to the managed file list and exit right away\n\
  \e[94m--managed-file-list <file>\e[0m    use <file> instead of the default value as the managed file list\n\
  \e[94m--show,-s\e[0m                     dry-run mode (i.e. just print but never execute commands)\n\
  \e[94m--dry-run,-n\e[0m                  pass `--dry-run` option to `rsync`\n\
  \e[94m--help,-h\e[0m                     show this help\n\
  \e[94m--\e[0m                            pass all of the trailing arguments to `rsync` as options\n\
";

        } else {

            cout << "\
Usage\n\
  yback [<option(s)>] [-- [<rsync option(s)>]]\n\
  \n\
Options\n\
  --bkrc,-b <file>              read <file> as a config file\n\
  --file,-f <file>              copy <file> to `/tmp` before any other operations\n\
  --tmp-directory <dir>         use <dir> instead of `/tmp` as the tmp directory\n\
  --add <file>                  add <file> to the managed file list before any other operations\n\
  --only-add <file>             add <file> to the managed file list and exit right away\n\
  --managed-file-list <file>    use <file> instead of the default value as the managed file list\n\
  --show,-s                     dry-run mode (i.e. just print but never execute commands)\n\
  --dry-run,-n                  pass `--dry-run` option to `rsync`\n\
  --help,-h                     show this help\n\
  --                            pass all of the trailing arguments to `rsync` as options\n\
";

        }

    }

    const char * colorize_if_isatty(const char *color) {
        if (isatty(1)) {
            return color;
        } else {
            return "";
        }
    }

    void lock() {

        if (misc::does_file_exist_without_expansion(prm::lock_file)) {
            cout << prm::colorize_if_isatty(color::fg_red_bright);
            cout << "Another session of `yback` is being executed.\n";
            cout << prm::colorize_if_isatty(color::color_end);
            cout << flush;
            errno = 1;
            return;
        }

        errno = 0; //This is needed since `realpath()` inside `misc::does_file_exist_without_expansion()` sets `errno` to some non-zero value if the file does not exist.

        ofstream ofs(prm::lock_file.c_str());
        if (!ofs) {
            cout << prm::colorize_if_isatty(color::fg_red_bright);
            cout << "Failed in locking.\n";
            cout << prm::colorize_if_isatty(color::color_end);
            cout << flush;
            errno = 1;
            return;
        }

        ofs.close();

    }

    void unlock() {
        remove(prm::lock_file.c_str());
    }

    //The argument `option` should not have preceding hyphen(s) since they are appropriately prepended inside this function.
    void print_option_parse_error(string option, const string &arg_to_option = "") {
        if (option.size() == 1) {
            option = "-" + option;
        } else {
            if (option[1] == ' ') {
                option = "-" + option;
            } else {
                option = "--" + option;
            }
        }
        cout << "An error occurred while parsing the option [ " << option;
        if (arg_to_option != "") {
            cout << " " << arg_to_option;
        }
        cout << " ].\n";
    }

    //This function checks validity of an option which takes a file name as its argument.
    bool check_validity_of_file_option(const string &option, const string &arg_to_option) {
        if (!misc::does_file_exist_without_expansion(arg_to_option)) {
            cout << prm::colorize_if_isatty(color::fg_red_bright);
            prm::print_option_parse_error(option, arg_to_option);
            cout << "The file [ " << arg_to_option << " ] does not exist.\n";
            cout << prm::colorize_if_isatty(color::color_end);
            cout << flush;
            return false;
        } else {
            return true;
        }
    }

    bool check_validity_of_directory_option(const string &option, const string &arg_to_option) {
        if (!misc::is_directory(arg_to_option)) {
            cout << prm::colorize_if_isatty(color::fg_red_bright);
            prm::print_option_parse_error(option, arg_to_option);
            cout << "The directory [ " << arg_to_option << " ] does not exist.\n";
            cout << prm::colorize_if_isatty(color::color_end);
            cout << flush;
            return false;
        } else {
            return true;
        }
    }

    void print_file_opening_error(const string &file) {
        cout << prm::colorize_if_isatty(color::fg_red_bright)
             << "Couldn't open the file [ " << file << " ].\n"
             << prm::colorize_if_isatty(color::color_end)
             << flush;
    }

    void print_command_failure(int exit_status) {
        cout << prm::colorize_if_isatty(color::fg_red_bright)
             << "The command exited with the exit status [ " << exit_status << " ].\n"
             << prm::colorize_if_isatty(color::color_end)
             << flush;
    }

    class BackupUnit {

        private:
            
            vector<string> source_list_;
            const string destination_;

            vector<string> option_to_rsync_list_;

        public:

            BackupUnit() { }

            BackupUnit(const string &destination, const vector<string> &option_to_rsync_list) : destination_(destination), source_list_(), option_to_rsync_list_(option_to_rsync_list) { }

            void add_source_(const string &source) {
                source_list_.push_back(source);
            }

            void add_rsync_option_(const string &option) {
                option_to_rsync_list_.push_back(option);
            }

            vector<string> create_backup_command_() const {

                if (source_list_.empty()) {
                    cout << prm::colorize_if_isatty(color::fg_red_bright)
                         << "The source which corresponds to the destination [ " << destination_ << " ] is not specified.\n"
                         << prm::colorize_if_isatty(color::color_end)
                         << flush; //`flush` is needed to immediately reflect the effect of `color_end`.
                    return vector<string>();
                }

                vector<string> command = {"rsync"};
                command.insert(command.end(), option_to_rsync_list_.begin(), option_to_rsync_list_.end());
                command.insert(command.end(), source_list_.begin(), source_list_.end());
                command.push_back(destination_);
                return command;

            }

            void print_() const {
                cout << prm::colorize_if_isatty(color::fg_blue_bright);
                cout << "Destination: [ " << destination_ << " ]\n";
                cout << "Source: ";
                misc::print_array(source_list_);
                cout << "Options: ";
                misc::print_array(option_to_rsync_list_);
                cout << prm::colorize_if_isatty(color::color_end)
                     << flush;
            }

            void print_destination_() const {
                cout << prm::colorize_if_isatty(color::fg_blue_bright)
                     << "-> \"" << destination_ << "\"\n"
                     << prm::colorize_if_isatty(color::color_end)
                     << flush;
            }

    };

}

int main(int argc, char **argv) {

    prm::lock();
    if (errno != 0) {
        return 1;
    }
    atexit(prm::unlock);

    //option parsing {

    vector<string> config_file_list; //-b,--bkrc
    vector<string> option_to_rsync_list; //all arguments after "--"
    bool is_show_mode = false; //-s,--show
    vector<string> copied_to_tmp_directory_file_list; //-f,--file
    string tmp_directory = prm::default_tmp_directory; //--tmp-directory
    vector<string> newly_managed_file_list; //--add,--only-add
    bool is_only_add_mode = false; //--only-add
    string managed_file_list_file = prm::default_managed_file_list_file; //--managed-file-list

    getopt_class::GetOpt go(argc, argv, prm::option_list);

    while (true) {

        go.parse_next_();

        if (go.end_()) {

            break;

        } else if (go.error_()) {

            go.print_error_();
            return 1;

        } else if (go.is_opt_()) {

            string option = go.get_opt_();

            if (option == "h" || option == "help") {
                prm::print_usage();
                return 0;
            } else if (option == "n" || option == "dry-run") {
                option_to_rsync_list.push_back("--dry-run");
                option_to_rsync_list.push_back("--info=stats"); //Without this, we have no visual way to know if it is dry-run mode or not.
            } else if (option == "s" || option == "show") {
                is_show_mode = true;
            } else if (option == "f" || option == "file") {
                if (!prm::check_validity_of_file_option(option, go.get_arg_())) {
                    return 1;
                }
                copied_to_tmp_directory_file_list.push_back(go.get_arg_());
            } else if (option == "tmp-directory") {
                if (!prm::check_validity_of_directory_option(option, go.get_arg_())) {
                    return 1;
                }
                tmp_directory = go.get_arg_();
            } else if (option == "add") {
                if (!prm::check_validity_of_file_option(option, go.get_arg_())) {
                    return 1;
                }
                newly_managed_file_list.push_back(realpath(go.get_arg_().c_str(), NULL)); //pushes the absolute path
            } else if (option == "only-add") {
                if (!prm::check_validity_of_file_option(option, go.get_arg_())) {
                    return 1;
                }
                newly_managed_file_list.push_back(realpath(go.get_arg_().c_str(), NULL));
                is_only_add_mode = true;
            } else if (option == "managed-file-list") {
                if (!prm::check_validity_of_file_option(option, go.get_arg_())) {
                    return 1;
                }
                managed_file_list_file = go.get_arg_();
            } else if (option == "b" || option == "bkrc") {
                if (!prm::check_validity_of_file_option(option, go.get_arg_())) {
                    return 1;
                }
                config_file_list.push_back(go.get_arg_());
            } else {
                return 1; //If the codes are free of bugs, you in principle never have any way to get here.
            }

        } else { //all arguments after "--"

            const string option_to_rsync = go.get_arg_();

            if (option_to_rsync[0] != '-') {
                cout << prm::colorize_if_isatty(color::fg_red_bright)
                     << "The non-option argument [ " << option_to_rsync << " ] is specified.\n"
                     << prm::colorize_if_isatty(color::color_end)
                     << flush;
                return 1;
            }

            option_to_rsync_list.push_back(go.get_arg_());

        }

    }

    //} option parsing

    //-b,--bkrc
    if (config_file_list.empty()) {
        cout << prm::colorize_if_isatty(color::fg_red_bright)
             << "No config file is specified. Use at least one -b/--bkrc option.\n"
             << prm::colorize_if_isatty(color::color_end)
             << flush;
        cout << "\n";
        prm::print_usage();
        return 1;
    }

    //-f,--file
    if (!copied_to_tmp_directory_file_list.empty()) {

        vector<string> command = {
                                   "rsync",
                                   "-au",
                                   "--info=name",
                                 };

        for (int i = 0; i < copied_to_tmp_directory_file_list.size(); ++i) {

            command.push_back(copied_to_tmp_directory_file_list[i]);

            cout << prm::colorize_if_isatty(color::fg_blue_bright)
                 << copied_to_tmp_directory_file_list[i] << " -> " << tmp_directory << "\n"
                 << prm::colorize_if_isatty(color::color_end)
                 << flush;

        }

        command.push_back(tmp_directory);

        if (!is_show_mode) {
            int exit_status = misc::exec(command);
            if (exit_status != 0) {
                prm::print_command_failure(exit_status);
                return exit_status;
            }
        }

    }

    //File Management
    //This is related to `--add` or `--only-add` option, but the list of managed files can generally be modified even when they are not specified.
    //
    //File management is the generalization of `-f` option.
    //If a file <file> is "manage"d, it is equivalent to specify `-f <file>` every time you launch `yback`.
    //File management works as below.
    //1. You can add files to the list of managed files, using `--add` option or `--only-add` option.
    //2. Every time you execute `yback`, the list is read and the files in it are automatically copied to `tmp_directory`.
    //To implement file management, we should handle the following two cases.
    //1. If a file in the list has been removed, we should also remove it from the list.
    //2. The list should never contain the same files. In other words, elements of the list should be unique.
    //The current implementation is as follows.
    //1. Read names of files of the existing list and those specified with `--add` or `--only-add` option into the same set.
    //   At that time, check the existence of the files and skip files which doesn't exist.
    //   Since we use a set, duplication check is not needed.
    //2. Write the contents of the list to the file `managed_file_list_file`.
    //3. Copy files to `tmp_directory` according to the set.
    if (!is_show_mode) {

        bool is_list_modified = false;

        cout << prm::colorize_if_isatty(color::fg_blue_bright)
             << "\n" << "[File Management]" << "\n"
             << prm::colorize_if_isatty(color::color_end)
             << flush;

        set<string> managed_file_set;

        //Processes files specified with `--add` or `--only-add` option.
        //Note that absolute paths are contained in `newly_managed_file_list` and that check of their existence has already been done.
        for (int i = 0; i < newly_managed_file_list.size(); ++i) {
            is_list_modified = true;
            cout << prm::colorize_if_isatty(color::fg_blue_bright)
                 << "Added the file [ " << newly_managed_file_list[i] << " ] to the managed file list.\n"
                 << prm::colorize_if_isatty(color::color_end)
                 << flush;
            managed_file_set.insert(newly_managed_file_list[i]);
        }

        ifstream ifs(managed_file_list_file.c_str());
        if (!ifs) {
            ifs.close();
            ofstream ofs(managed_file_list_file.c_str()); //tries to create the file
            if (!ofs) {
                prm::print_file_opening_error(managed_file_list_file);
                return 1;
            }
            ofs.close();
            ifs.open(managed_file_list_file); //re-opens the created file
            if (!ifs) {
                prm::print_file_opening_error(managed_file_list_file);
                return 1;
            }
        }

        while (true) {

            string buf;
            getline(ifs, buf);

            if (!ifs) {
                break;
            }

            //If a file does not exist, it is not inserted to the set.
            //This means the file is removed from the managed file list since the set will finally be written back to `managed_file_list_file`.
            if (misc::does_file_exist_without_expansion(buf)) {
                managed_file_set.insert(buf);
            } else {
                is_list_modified = true;
                cout << prm::colorize_if_isatty(color::fg_red_bright)
                     << "Removed the file [ " << buf << " ] from the managed file list.\n"
                     << prm::colorize_if_isatty(color::color_end)
                     << flush;
            }

        }

        ifs.close();

        if (managed_file_set.size() == 0) {

            if (is_list_modified) {

                //Makes `managed_file_list_file` empty.
                ofstream ofs(managed_file_list_file.c_str());
                if (!ofs) {
                    prm::print_file_opening_error(managed_file_list_file);
                    return 1;
                }
                ofs.close();

            }

            goto end_of_file_management;

        }

        vector<string> command = {
                                   "rsync",
                                   "-au",
                                   "--info=name",
                                 };

        if (is_list_modified) {

            ofstream ofs(managed_file_list_file.c_str());
            if (!ofs) {
                prm::print_file_opening_error(managed_file_list_file);
                return 1;
            }

            for (const string &file : managed_file_set) {
                ofs << file << "\n";
                command.push_back(file);
            }
            command.push_back(tmp_directory);

            ofs.close();

        } else {

            for (const string &file : managed_file_set) {
                command.push_back(file);
            }
            command.push_back(tmp_directory);

        }

        if (!is_show_mode) {
            int exit_status = misc::exec(command);
            if (exit_status != 0) {
                prm::print_command_failure(exit_status);
                return exit_status;
            }
        }

        if (is_only_add_mode) {
            return 0;
        }

        cout << "\n";

    }
    end_of_file_management:

    //for debug
    //Adds indents to outputs to make the structure of recursion visible.
    #ifndef NDEBUG
        unsigned recursion_level = 0;
        function<void ()> output_indent = [&recursion_level]() -> void {
            for (int i = 0; i < recursion_level; ++i) {
                cout << "    ";
            }
        };
    #endif

    //This is an array of backup units.
    //Each element is a `prm::BackupUnit` class object whose members represent sources, options to rsync, etc.
    vector<prm::BackupUnit> backup_unit_list;

    map<string, string> variable_list;

    function<int (const string &)> parse_config_file; //The definition of this function is given right after the definition of `parse_line()`.

    function<int (const string)> parse_line = [&](const string &original_line) -> /* exit status */ int {

        const string line = misc::omit_whitespaces_from_string(original_line); //omits preceding and trailing whitespaces

        if (line.empty()) { //ignores empty lines
            return 0;
        }

        const char first_character = line[0];
        const string line_wo_prefix = (line.size() > 2 ? string(line.begin() + 2, line.end()) : string());

        if (first_character == '?') {
            
            string reply;
            cout << line_wo_prefix << "(y/[n]): ";
            cin >> reply;

            if (!cin) {
                cout << prm::colorize_if_isatty(color::fg_red_bright)
                     << "\nThe input was invalid.\n"
                     << prm::colorize_if_isatty(color::color_end)
                     << flush;
                return 1;
            } else if (reply == "y") {
                cout << prm::colorize_if_isatty(color::fg_blue_bright)
                     << "Proceeding.\n"
                     << prm::colorize_if_isatty(color::color_end)
                     << flush;
            } else {
                cout << prm::colorize_if_isatty(color::fg_blue_bright)
                     << "The operation cancelled.\n"
                     << prm::colorize_if_isatty(color::color_end)
                     << flush;
                return 1;
            }

        } else if (first_character == '>') {
            
            const string destination = misc::word_expansion(line_wo_prefix)[0];
            
            //It is relatively difficult to know in advance if the destination exists since it may be on a remote host.
            //Currently, we interpret a destination as a remote directory if it contains a colon ':'.
            //Such destinations are not checked for existence.
            if (destination.find(':') != string::npos || misc::does_file_exist_without_expansion(destination)) {
                backup_unit_list.push_back(prm::BackupUnit(destination, option_to_rsync_list));
            } else {
                cout << prm::colorize_if_isatty(color::fg_red_bright)
                     << "The destination [ " << destination << " ] does not exist.\n"
                     << prm::colorize_if_isatty(color::color_end)
                     << flush;
                return 1;
            }

        } else if (first_character == '<') {

            if (backup_unit_list.empty()) {
                cout << prm::colorize_if_isatty(color::fg_red_bright)
                     << "The source [ " << line_wo_prefix << " ] is specified but the destination is empty in the current context.\n"
                     << prm::colorize_if_isatty(color::color_end)
                     << flush;
                return 1;
            }

            const string source = misc::word_expansion(line_wo_prefix)[0];
            
            if (misc::does_file_exist_without_expansion(source)) {
                backup_unit_list.back().add_source_(source);
            } else {
                cout << prm::colorize_if_isatty(color::fg_red_bright)
                     << "The source [ " << source << " ] does not exist.\n"
                     << prm::colorize_if_isatty(color::color_end)
                     << flush;
                return 1;
            }

        } else if (first_character == '-') {

            if (backup_unit_list.empty()) {
                cout << prm::colorize_if_isatty(color::fg_red_bright)
                     << "The destination specific option [ " << line << " ] is specified but the source is empty in the current context.\n"
                     << prm::colorize_if_isatty(color::color_end)
                     << flush;
                return 1;
            }

            backup_unit_list.back().add_rsync_option_(misc::word_expansion(line)[0]);

        } else if (first_character == '+') {

            const string config_file = misc::word_expansion(line_wo_prefix)[0];

            #ifndef NDEBUG
                cout << prm::colorize_if_isatty(color::fg_blue_bright)
                     << "Recursively sourced the config file [ " << config_file << " ].\n"
                     << prm::colorize_if_isatty(color::color_end)
                     << flush;
                ++recursion_level;
            #endif

            int exit_status = parse_config_file(config_file);
            if (exit_status != 0) {
                return exit_status;
            }

        } else if (first_character == '$') {

            cout << prm::colorize_if_isatty(color::fg_blue_bright)
                 << line << "\n"
                 << prm::colorize_if_isatty(color::color_end)
                 << flush;

            if (!is_show_mode) {
                int exit_status = system(line_wo_prefix.c_str());
                if (exit_status != 0) {
                    cout << prm::colorize_if_isatty(color::fg_red_bright)
                         << "An error occurred while executing the command.\n"
                         << prm::colorize_if_isatty(color::color_end)
                         << flush;
                    return exit_status;
                }
            }

            cout << "\n";

        } else if (first_character == '%') {

            istringstream is(line_wo_prefix);

            if (line_wo_prefix.find('=') == string::npos) { //variable references

                vector<string> variable_reference_list;

                while (true) {

                    string variable_name;
                    is >> variable_name;

                    if (!is) {
                        break;
                    }

                    if (!variable_list.count(variable_name)) {
                        cout << prm::colorize_if_isatty(color::fg_red_bright)
                             << "The variable [ " << variable_name << " ] was referenced but not defined.\n"
                             << prm::colorize_if_isatty(color::color_end)
                             << flush;
                        return 1;
                    }

                    variable_reference_list.push_back(variable_name);

                }

                for (int i = 0; i < variable_reference_list.size(); ++i) {
                    int exit_status = parse_line(variable_list[variable_reference_list[i]]);
                    if (exit_status != 0) {
                        return exit_status;
                    }
                }

            } else { //variable definitions

                function<void ()> print_error = [&line_wo_prefix]() -> void {
                    cout << prm::colorize_if_isatty(color::fg_red_bright)
                         << "An error occurred while parsing the variable definition [ " << line_wo_prefix << " ].\n"
                         << prm::colorize_if_isatty(color::color_end)
                         << flush;
                };

                string variable_name;
                string equal_sign;
                string variable_value;

                is >> variable_name;
                if (!is) {
                    print_error();
                    return 1;
                }

                is >> equal_sign;
                if (!is || equal_sign != "=") {
                    print_error();
                    return 1;
                }

                getline(is, variable_value);
                if (!is) {
                    print_error();
                    return 1;
                }

                if (variable_list.count(variable_name)) {
                    cout << prm::colorize_if_isatty(color::fg_red_bright)
                         << "The variable [ " << variable_name << " ] has already been defined.\n"
                         << prm::colorize_if_isatty(color::color_end)
                         << flush;
                    return 1;
                }

                variable_list[variable_name] = string(variable_value.begin() + 1 /* This `1` omits a preceding space. */, variable_value.end());

            }

        } else if (first_character == '#') {

            ;

        } else {

            cout << prm::colorize_if_isatty(color::fg_red_bright)
                 << "An error occurred while parsing the line [ " << line << " ].\n"
                 << prm::colorize_if_isatty(color::color_end)
                 << flush;
            return 1;

        }

        return 0;

    };

    parse_config_file = [&](const string &config_file_name) -> /* exit status */ int {

        #ifndef NDEBUG
            output_indent();
            cout << prm::colorize_if_isatty(color::fg_blue_bright)
                 << "Started parsing the config file [ " << config_file_name << " ].\n"
                 << prm::colorize_if_isatty(color::color_end)
                 << flush;
        #endif

        ifstream ifs(config_file_name.c_str());
        if (!ifs) {
            prm::print_file_opening_error(config_file_name);
            return 1;
        }

        while (true) {

            string line;

            getline(ifs, line);
            if (!ifs) {
                break;
            }

            int exit_status = parse_line(line);
            if (exit_status != 0) {
                return exit_status;
            }

        }

        ifs.close();

        #ifndef NDEBUG
            output_indent();
            --recursion_level;
            cout << prm::colorize_if_isatty(color::fg_blue_bright)
                 << "Ended parsing the config file [ " << config_file_name << " ].\n"
                 << prm::colorize_if_isatty(color::color_end)
                 << flush;
        #endif

        return 0;

    };

    for (int i = 0; i < config_file_list.size(); ++i) {
        int exit_status = parse_config_file(config_file_list[i]);
        if (exit_status != 0) {
            return exit_status;
        }
    }

    #ifndef NDEBUG
        for (int i = 0; i < backup_unit_list.size(); ++i) {
            cout << "-----\n";
            backup_unit_list[i].print_();
        }
        cout << "-----\n";
    #endif

    for (int i = 0; i < backup_unit_list.size(); ++i) {

        const vector<string> backup_command = backup_unit_list[i].create_backup_command_();
        if (backup_command.empty()) {
            return 1;
        }

        backup_unit_list[i].print_destination_();

        if (is_show_mode) {

            misc::print_array(backup_command);

        } else {

            int exit_status = misc::exec(backup_command);
            if (exit_status != 0) {
                cout << prm::colorize_if_isatty(color::fg_red_bright)
                     << "An error occurred while executing the backup ";
                misc::print_array(backup_command, /* should_append_newline = */ false);
                cout << ".\n"
                     << prm::colorize_if_isatty(color::color_end)
                     << flush;
                return exit_status;
            }

            if (i != backup_unit_list.size() - 1) {
                cout << "\n";
            }

        }

    }

}

// vim: spell

