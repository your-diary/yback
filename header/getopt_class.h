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

                //可能なオプションと、その属性(属性は、引数の取り方を表す)
                vector<string> option_list_;
                vector<option_attribute_> option_attribute_list_;

                //parse_next_()の度に値が変わり得るもの {

                bool error_flag_ = false;
                string error_string_;

                bool is_option_ = false;
                bool has_argument_ = false; //オプションに対応する引数が与えられているか

                string option_;
                string option_argument_; //オプションに対する引数(例えば`--file <file>`で言う`<file>`)

                //} parse_next_()の度に値が変わり得るもの

                bool has_parse_ended_ = false;

            private:

                //`str`が有効なオプションであるか
                //返り値: 有効なオプションである場合は、`option_list_`内のインデックス
                //        そうでない場合は、-1
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
                        error_string_ = "引数が一つも与えられていません。";
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
                            error_string_ = "`option_list`の要素[ " + option_list[i] + " ]が不正です。";
                        }

                    }

                    if (misc::has_duplicate_element(option_list_.begin(), option_list_.end(), true)) {
                        error_flag_ = true;
                        error_string_ = "`option_list`の要素に重複があります。";
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

                    //static変数 {

                    static unsigned index = -1;

                    static bool should_interpret_as_non_option = false; //単なる`--`が与えられたとき、それ以降の引数は非オプションとして扱う

                    static bool is_parsing_combination_of_short_option = false; //`-auv`のような、短いオプションの組み合わせを解析中であるか
                    static int index_inside_combination; //短いオプションの組み合わせにおいて、次にどの文字を解析するか(例えば`-auv`だと、"a"が0に対応する)

                    //} static変数

                    if (error_flag_) { //既にエラーフラグが立っている場合(例えばコンストラクタが失敗している場合)
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

                    if (is_parsing_combination_of_short_option) { //短いオプションの組み合わせの解析

                        parse_combination:

                        is_option_ = true;

                        const string option = to_string(target_str[/* length of '-' */ 1 + index_inside_combination]);

                        const int option_index = find_from_option_list_(option);

                        if (option_index == -1) {

                            error_flag_ = true;
                            error_string_ = "オプション[ " + target_str + " ]の解析中にエラーが発生しました。オプション[ -" + option + " ]は不正です。";

                        } else {

                            option_ = option;

                            //`option_`が引数を必ず要するオプションの場合、エラーとする
                            if (!(option_attribute_list_[option_index] == normal_ || option_attribute_list_[option_index] == zero_or_one_arg_)) {
                                error_flag_ = true;
                                error_string_ = "オプション[ " + target_str + " ]の解析中にエラーが発生しました。オプション[ -" + option + " ]は引数を取ります。";
                                return;
                            }

                            ++index_inside_combination;

                            if (/* length of '-' */ 1 + (index_inside_combination + 1) > target_str.size()) { //短いオプションの組み合わせの解析が終わったとき
                                is_parsing_combination_of_short_option = false;
                            }

                        }

                    } else if (!should_interpret_as_non_option) { //`-h`や`--file <file>`といった単体のオプションの解析

                        is_option_ = true;

                        if (target_str == "-") {

                            error_flag_ = true;
                            error_string_ = "単なる[ " + target_str + " ]が指定されました。";

                        } else if (target_str == "--") {

                            should_interpret_as_non_option = true;

                            parse_next_(); //"--"自体は無視したいため、ここで再帰呼び出しする

                        } else if (misc::does_start_with(target_str, "-") || misc::does_start_with(target_str, "--")) {

                            const bool does_start_with_double_hyphen = misc::does_start_with(target_str, "--");

                            const string option(target_str.begin() + (does_start_with_double_hyphen ? 2 : 1), target_str.end());

                            const int option_index = find_from_option_list_(option);

                            if (option_index == -1) {

                                if (does_start_with_double_hyphen) {

                                    error_flag_ = true;
                                    error_string_ = "不正なオプション[ " + target_str + " ]が指定されました。";

                                } else {

                                    is_parsing_combination_of_short_option = true;
                                    index_inside_combination = 0;
                                    goto parse_combination;

                                }

                            } else {

                                option_ = option;

                                if (!does_start_with_double_hyphen && option_.size() > 1) { //"-umount"のようなケース(正しくは"--umount")
                                    error_flag_ = true;
                                    error_string_ = "オプション[ " + target_str + " ]の形式が不正です。正しくは[ -" + target_str + " ]としてください。";
                                    return;
                                }

                                if (option_attribute_list_[option_index] == zero_or_one_arg_ || option_attribute_list_[option_index] == one_arg_) { //オプションに対する引数の処理

                                    if (index < argv_.size() - 1 && !misc::does_start_with(argv_[index + 1], "-")) { //引数があるとき
                                        has_argument_ = true;
                                        option_argument_ = argv_[index + 1];
                                        ++index;
                                    } else if (option_attribute_list_[option_index] == one_arg_) { //引数が必要なのに無いとき
                                        error_flag_ = true;
                                        error_string_ = "オプション[ " + target_str + " ]に対する引数が指定されていません。";
                                    }

                                }

                            }

                        } else {

                            goto interpret_as_non_option;

                        }

                    } else {

                        interpret_as_non_option:

                        is_option_ = false; //これを消してはならない(goto文でここに飛んでくる場合に必要となる)

                        option_argument_ = target_str;

                    }

                }

        };

    }

#endif

