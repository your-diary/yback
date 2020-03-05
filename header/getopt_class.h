#ifndef is_getopt_class_included

    #define is_getopt_class_included

    #include <iostream>
    #include <set>
    #include <vector>
    #include <unistd.h>
    #include "./misc.h"
    #include "./color.h"

    namespace getopt_class {

        using namespace std;

        class GetOpt {

            private:

                vector<string> argv_;

                enum option_attribute_ { normal_, zero_or_one_arg_, one_arg_ };

                vector<string> option_list_;
                vector<option_attribute_> option_attribute_list_;

                bool error_flag_ = false;
                string error_string_;

                bool is_option_ = false;
                bool has_argument_ = false;

                string option_;
                string option_argument_;

                bool has_parse_ended_ = false;

            private:

                int find_from_option_list_(const string &str) {

                    for (int i = 0; i < option_list_.size(); ++i) {
                        if (option_list_[i] == str) {
                            return i;
                        }
                    }

                    return -1;

                }

            public:
                
                GetOpt(int argc, char **argv, const vector<string> &option_list, bool should_forbid_zero_arg = false) {

                    for (int i = 1; i < argc; ++i) {
                        argv_.push_back(argv[i]);
                    }
                    if (should_forbid_zero_arg && argv_.size() == 0) {
                        error_flag_ = true;
                        error_string_ = "No argument is specified.";
                    }

                    for (int i = 0; i < option_list.size(); ++i) {

                        option_list_.push_back(string(option_list[i].begin() + 1, option_list[i].end()));

                        if (misc::does_start_with(option_list[i], "--")) {
                            option_list_.back().assign(option_list[i].begin() + 2, option_list[i].end());
                            option_attribute_list_.push_back(normal_);
                        } else if (misc::does_start_with(option_list[i], "-")) {
                            option_attribute_list_.push_back(normal_);
                        } else if (misc::does_start_with(option_list[i], "?")) {
                            option_attribute_list_.push_back(zero_or_one_arg_);
                        } else if (misc::does_start_with(option_list[i], "+")) {
                            option_attribute_list_.push_back(one_arg_);
                        } else {
                            error_flag_ = true;
                            error_string_ = "The element [ " + option_list[i] + " ] of `option_list` is invalid.";
                        }

                    }

                    if (misc::has_duplicate_element(option_list_.begin(), option_list_.end(), true)) {
                        error_flag_ = true;
                        error_string_ = "`option_list` has duplicated elements.";
                    }

                }

                bool is_opt_() const {
                    return is_option_;
                }

                string get_opt_() const {
                    return option_;
                }

                bool has_arg_() const {
                    return has_argument_;
                }

                string get_arg_() const {
                    return option_argument_;
                }

                bool end_() const {
                    return has_parse_ended_;
                }

                bool error_() const {
                    return error_flag_;
                }

                void print_error_() const {
                    if (isatty(1)) {
                        cout << color::fg_red << "GetOpt: " << error_string_ << color::color_end << "\n";
                    } else {
                        cout << "GetOpt: " << error_string_ << "\n";
                    }
                }

                void parse_next_() {

                    //static variables {

                    static unsigned index = -1;

                    static bool should_interpret_as_non_option = false; //When a simple `--` is given, we interpret the trailing arguments as non-options.

                    static bool is_parsing_combination_of_short_option = false; //If or not we are parsing a combination of short options like `-auv`.
                    static int index_inside_combination; //In a combination, which character to parse in the next step. (For example, in `-auv`, "a" corresponds to 0.)

                    //} static variables

                    if (error_flag_) {
                        return;
                    }

                    if (!is_parsing_combination_of_short_option) {
                        ++index;
                    }
                    if (index >= argv_.size()) {
                        has_parse_ended_ = true;
                        return;
                    }

                    is_option_ = false;
                    has_argument_ = false;
                    option_ = "";
                    option_argument_ = "";

                    const string target_str = argv_[index];

                    if (is_parsing_combination_of_short_option) { //parses a combination of short options

                        parse_combination:

                        is_option_ = true;

                        const string option = to_string(target_str[/* length of '-' */ 1 + index_inside_combination]);

                        const int option_index = find_from_option_list_(option);

                        if (option_index == -1) {

                            error_flag_ = true;
                            error_string_ = "An error occurred while parsing the option [ " + target_str + " ]. The option [ -" + option + " ] is invalid.";

                        } else {

                            option_ = option;

                            if (!(option_attribute_list_[option_index] == normal_ || option_attribute_list_[option_index] == zero_or_one_arg_)) {
                                error_flag_ = true;
                                error_string_ = "An error occurred while parsing the option [ " + target_str + " ]. The option [ -" + option + " ] takes an argument.";
                                return;
                            }

                            ++index_inside_combination;

                            if (/* length of '-' */ 1 + (index_inside_combination + 1) > target_str.size()) {
                                is_parsing_combination_of_short_option = false;
                            }

                        }

                    } else if (!should_interpret_as_non_option) {

                        is_option_ = true;

                        if (target_str == "-") {

                            error_flag_ = true;
                            error_string_ = "A simple [ " + target_str + " ] was specified.";

                        } else if (target_str == "--") {

                            should_interpret_as_non_option = true;

                            parse_next_(); //calls recursively to ignore "--" itself

                        } else if (misc::does_start_with(target_str, "-") || misc::does_start_with(target_str, "--")) {

                            const bool does_start_with_double_hyphen = misc::does_start_with(target_str, "--");

                            const string option(target_str.begin() + (does_start_with_double_hyphen ? 2 : 1), target_str.end());

                            const int option_index = find_from_option_list_(option);

                            if (option_index == -1) {

                                if (does_start_with_double_hyphen) {

                                    error_flag_ = true;
                                    error_string_ = "An invalid option [ " + target_str + " ] was specified.";

                                } else {

                                    is_parsing_combination_of_short_option = true;
                                    index_inside_combination = 0;
                                    goto parse_combination;

                                }

                            } else {

                                option_ = option;

                                if (!does_start_with_double_hyphen && option_.size() > 1) { //handles cases such as "-umount" (The correct string is "--umount".)
                                    error_flag_ = true;
                                    error_string_ = "The form of the option [ " + target_str + " ] is invalid. It should be [ -" + target_str + " ].";
                                    return;
                                }

                                if (option_attribute_list_[option_index] == zero_or_one_arg_ || option_attribute_list_[option_index] == one_arg_) { //handles arguments to an option

                                    if (index < argv_.size() - 1 && !misc::does_start_with(argv_[index + 1], "-")) {
                                        has_argument_ = true;
                                        option_argument_ = argv_[index + 1];
                                        ++index;
                                    } else if (option_attribute_list_[option_index] == one_arg_) {
                                        error_flag_ = true;
                                        error_string_ = "An argument to the option [ " + target_str + " ] is not specified.";
                                    }

                                }

                            }

                        } else {

                            goto interpret_as_non_option;

                        }

                    } else {

                        interpret_as_non_option:

                        is_option_ = false; //This line is needed. (Used when jumping here via `goto` statement.)

                        option_argument_ = target_str;

                    }

                }

        };

    }

#endif

