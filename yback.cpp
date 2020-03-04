#include <iostream>
using namespace std;
#include <cassert>
#include <ynn.h>

// #define NDEBUG

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

    const string default_managed_file_list_file = ynn::word_expansion("~/.yback_managed_file_list")[0];

    void print_usage() {

        if (isatty(1)) {

            cout << "\
\e[;100mUsage\e[0m\n\
  back [<option(s)>] [<rsync option(s)>]\n\
  \n\
\e[;100mOptions\e[0m\n\
    \e[94m--show,-s\e[0m: コマンドを実際には実行しない(dry-run)\n\
    \e[94m--file,-f\e[0m: バックアップの実行前に、続くファイル引数を一時フォルダへ複製する\n\
    \e[94m    --add\e[0m: 自動管理ファイルリストに、続くファイル引数を追加する\n\
   \e[94m--only-add\e[0m: 同上だが、実際のバックアップまでは行わない\n\
    \e[94m--bkrc,-b\e[0m: デフォルトの設定ファイルに加え、続くファイル引数も読む\n\
 \e[94m--dry-run,-n\e[0m: rsyncに--dry-runオプションを渡す\n\
    \e[94m--help,-h\e[0m: ヘルプを表示する\n\
    \e[94m       --\e[0m: 次以降の引数を強制的にrsyncオプションとする\n\
";

        } else {

            cout << "\
Usage\n\
  back [<option(s)>] [<rsync option(s)>]\n\
  \n\
Options\n\
    --show,-s: コマンドを実際には実行しない(dry-run)\n\
    --file,-f: バックアップの実行前に、続くファイル引数を一時フォルダへ複製する\n\
        --add: 自動管理ファイルリストに、続くファイル引数を追加する\n\
   --only-add: 同上だが、実際のバックアップまでは行わない\n\
    --bkrc,-b: デフォルトの設定ファイルに加え、続くファイル引数も読む\n\
 --dry-run,-n: rsyncに--dry-runオプションを渡す\n\
    --help,-h: ヘルプを表示する\n\
           --: 次以降の引数を強制的にrsyncオプションとする\n\
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
        if (!ynn::does_file_exist(arg_to_option)) {
            cout << prm::colorize_if_isatty(ynn::fg_red_bright);
            prm::print_option_parse_error(option, arg_to_option);
            cout << "The file [ " << arg_to_option << " ] does not exist.\n";
            cout << prm::colorize_if_isatty(ynn::color_end);
            cout << flush;
            return false;
        } else {
            return true;
        }
    }

    bool check_validity_of_directory_option(const string &option, const string &arg_to_option) {
        if (!ynn::is_directory(arg_to_option)) {
            cout << prm::colorize_if_isatty(ynn::fg_red_bright);
            prm::print_option_parse_error(option, arg_to_option);
            cout << "The directory [ " << arg_to_option << " ] does not exist.\n";
            cout << prm::colorize_if_isatty(ynn::color_end);
            cout << flush;
            return false;
        } else {
            return true;
        }
    }

    void print_file_opening_error(const string &file) {
        cout << prm::colorize_if_isatty(ynn::fg_red_bright)
             << "Couldn't open the file [ " << file << " ].\n"
             << prm::colorize_if_isatty(ynn::color_end)
             << flush;
    }

    void print_command_failure(int exit_status) {
        cout << prm::colorize_if_isatty(ynn::fg_red_bright)
             << "The command exited with the exit status [ " << exit_status << " ].\n"
             << prm::colorize_if_isatty(ynn::color_end)
             << flush;
    }



//     const unsigned path_length = 1000; //各バックアップ元のパスの最大長
// 
//     const string source_list_file_parent_dir = "/home/ynn/Scripts/";
//     const string source_list_file_prefix = "back_";
//     const string source_list_file_extension = ".txt";
// 
//     vector<string> destination_list;
//     vector<bool> is_sync_mode_list; //i番目のバックアップ先に同期モードを対応させるか(同期モードに対応するとき、-a -uオプションの代わりに-a --deleteオプションを用いる。つまり、バックアップ元とバックアップ先と同一内容とする)
// 
//     const string tmp_directory_path = "/home/ynn/tmp";
// 
//     const string base_command_wo_backup = "rsync -au --info=name --log-file=\"${RSYNC_LOG}\" ";
//     const string base_command = base_command_wo_backup + "--backup ";
// 
//     const string base_command_when_sync_mode = "rsync -a --delete --info=name --log-file=\"${RSYNC_LOG}\" ";

}

int main(int argc, char **argv) {

    //option parsing {

    vector<string> config_file_list; //-b,--bkrc
    vector<string> option_to_rsync_list; //all arguments after "--"
    bool is_show_mode = false; //-s,--show
    vector<string> copied_to_tmp_directory_file_list; //-f,--file
    string tmp_directory = prm::default_tmp_directory; //--tmp-directory
    vector<string> newly_managed_file_list; //--add,--only-add
    bool is_only_add_mode = false; //--only-add
    string managed_file_list_file = prm::default_managed_file_list_file; //--managed-file-list

    ynn::GetOpt go(argc, argv, prm::option_list);

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
                newly_managed_file_list.push_back(go.get_arg_());
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
                cout << prm::colorize_if_isatty(ynn::fg_red_bright)
                     << "A non-option argument [ " << option_to_rsync << " ] is specified.\n"
                     << prm::colorize_if_isatty(ynn::color_end)
                     << flush; //`flush` is needed to immediately reflect the effect of `color_end`.
                return 1;
            }

            option_to_rsync_list.push_back(go.get_arg_());

        }

    }

    //} option parsing

    //-f,--file
    if (!copied_to_tmp_directory_file_list.empty()) {

        vector<string> command = {
                                   "rsync",
                                   "-au",
                                   "--info=name",
                                 };

        for (int i = 0; i < copied_to_tmp_directory_file_list.size(); ++i) {

            command.push_back(copied_to_tmp_directory_file_list[i]);

            cout << prm::colorize_if_isatty(ynn::fg_blue_bright)
                 << copied_to_tmp_directory_file_list[i] << " -> " << tmp_directory << "\n"
                 << prm::colorize_if_isatty(ynn::color_end)
                 << flush;

        }

        command.push_back(tmp_directory);

        if (!is_show_mode) {
            int exit_status = ynn::exec(command);
            if (exit_status != 0) {
                prm::print_command_failure(exit_status);
                return exit_status;
            }
        }

    }

    //File Management
    //This is related to the `--add` or the `--only-add` option, but the list of managed files can generally be modified even when they are not specified.
    //
    //File management is a generalization of the `-f` option.
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

        cout << prm::colorize_if_isatty(ynn::fg_blue_bright)
             << "\n" << "[File Management]" << "\n"
             << prm::colorize_if_isatty(ynn::color_end)
             << flush;

        set<string> managed_file_set;

        //Processes files specified with `--add` or `--only-add` option.
        //Note that absolute paths are contained in `newly_managed_file_list` and that check of their existence has already been done.
        for (int i = 0; i < newly_managed_file_list.size(); ++i) {
            is_list_modified = true;
            cout << prm::colorize_if_isatty(ynn::fg_blue_bright)
                 << "Added the file [ " << newly_managed_file_list[i] << " ] to the managed file list.\n"
                 << prm::colorize_if_isatty(ynn::color_end)
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
            if (ynn::does_file_exist(buf)) {
                managed_file_set.insert(buf);
            } else {
                is_list_modified = true;
                cout << prm::colorize_if_isatty(ynn::fg_red_bright)
                     << "Removed the file [ " << buf << " ] from the managed file list.\n"
                     << prm::colorize_if_isatty(ynn::color_end)
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
            int exit_status = ynn::exec(command);
            if (exit_status != 0) {
                prm::print_command_failure(exit_status);
                return exit_status;
            }
        }

        if (is_only_add_mode) {
            return 0;
        }

    }
    end_of_file_management:

    cout << "Done.\n";

//     char source_path_tmp[prm::path_length];
//     char *p_source_path_tmp = source_path_tmp;
// 
//     string source_list; //クォーティングしたバックアップ元をスペース区切りで並べたもの
// 
//     //デバッグ用
//     //再帰呼び出しの構造を分かりやすくするために、出力にインデントを加える
//     unsigned recursion_level = 0;
//     auto output_indent = [&recursion_level]() {
//         for (int k = 0; k < recursion_level; ++k) {
//             cout << "\t";
//         }
//     };
// 
//     //設定ファイル中の「別の設定ファイルを読む」という'+'コマンドに対応する必要がある
//     //過去の版では、動的なfor文を使った実装をしており、+コマンドに到達した時点で対象ファイルをキューに追加し、現在のファイルは取り敢えず読み進めるという手を取っていた(ただし、ここの「キュー」は単に構造を指しており、std::queueを意味していない)
//     //この実装でも、長いこと問題は生じていなかったが、これではオプションの上書きが正常に行われないことが発覚した
//     //例えば、ファイルAに「ファイルBを読む」(命令1)および「--suffix=Aを設定する」(命令2)と書かれており、ファイルBに「--suffix=Bを設定する」(命令3)と書かれている場合、命令は1 -> 2 -> 3の順番で読まれ、最終的に有効となるのは命令3である
//     //しかしながら、ファイルAの意図としては、ファイルBの設定に対する追加または削除を行いたいのであるから、命令は1 -> 3 -> 2の順番で読まれ、最終的に命令2が有効化されるのが自然である
//     //これを実現するためには、+コマンドに達した時点で即座に対象ファイルの読み取りを始め、それが終わったら、また読み取り前の位置に戻ってくれば良い
//     //これには再帰関数を用いるのが簡単であるため、再帰的なラムダ式を採用することにした
//     //なお、もはやsource_list_fileをベクトルで保持する必要は本質的には無いが、--bkrcオプションがあるため、単なる文字列に変えてしまうわけにもいかない(例えば、source_list_fileを単なる文字列にして、--bkrcオプションの引数は別の文字列で保持し、後者が空でなければそちらに対してもparse_configuration_file()を呼び出す、などとすれば、ベクトルを廃止できる)
// 
//     function<int (const char *configuration_file_name)> parse_configuration_file = [&](const char *configuration_file_name) -> /* exit status */ int {
// 
// //         if (is_show_mode) {
// //             output_indent();
// //             cout << "設定ファイル[ " << configuration_file_name << " ]の読み取りを開始しました。\n";
// //         }
// 
//         ifstream ifs(configuration_file_name);
//         if (!ifs) {
//             print_file_not_exist_error(configuration_file_name);
//             return 1;
//         }
// 
//         while (true) {
// 
//             ifs.getline(p_source_path_tmp, prm::path_length - 1);
//             if (!ifs) {
//                 break;
//             }
// 
//             const char first_character = p_source_path_tmp[0];
//             const char second_character = p_source_path_tmp[1];
//             const string content_wo_prefix = &(p_source_path_tmp[2]); //行の内容の三文字目以降
// 
//             if (first_character == '!') {
// 
//                 break;
// 
//             } else if (first_character == '?') {
//                 
//                 string buf;
//                 cout << content_wo_prefix << "(y/n): ";
//                 cin >> buf;
// 
//                 if (!cin) {
//                     cout << "\n入力が不正です。\n";
//                     return 1;
//                 } else if (buf[0] == 'y' && buf.size() == 1) {
//                     cout << "コマンドの実行を続行します。\n";
//                 } else {
//                     cout << "コマンドの実行をキャンセルしました。\n";
//                     return 1;
//                 }
// 
//             } else if (first_character == '-') {
// 
//                 prm::option_to_rsync += p_source_path_tmp + string(" ");
// 
//             } else if (first_character == '+') {
// 
//                 cout << ynn::fg_blue_bright << "+ " << content_wo_prefix << ynn::color_end << "\n\n";
// 
//                 if (is_show_mode) {
//                     ++recursion_level;
//                 }
// 
//                 int exit_status = parse_configuration_file(content_wo_prefix.c_str()); //再帰的呼び出し
//                 if (exit_status != 0) {
//                     return 1;
//                 }
// 
//             } else if (first_character == '%') {
// 
//                 const string command_tmp = prm::base_command_wo_backup + content_wo_prefix + " " + prm::tmp_directory_path;
//                 cout << ynn::fg_blue_bright << "% [" << content_wo_prefix << " -> " << prm::tmp_directory_path << "]" << ynn::color_end << "\n";
//                 if (!is_show_mode) { system(command_tmp.c_str()); }
//                 cout << "\n";
// 
//             } else if (first_character == '^') {
// 
//                 if (is_show_mode) {
//                     cout << ynn::fg_blue_bright << "^ [" << content_wo_prefix << "]" << ynn::color_end << "\n";
//                     cout << "\n";
//                 } else {
//                     const string command_tmp = prm::base_command_wo_backup + content_wo_prefix;
//                     system(command_tmp.c_str());
//                 }
// 
//             } else if (first_character == '$') {
// 
//                 cout << ynn::fg_blue_bright << p_source_path_tmp << ynn::color_end << "\n";
//                 if (!is_show_mode) { system(content_wo_prefix.c_str()); }
//                 cout << "\n";
// 
//             } else if (first_character == '>') {
// 
//                 if (is_show_mode) {
//                     cout << ynn::fg_blue_bright << p_source_path_tmp << ynn::color_end << "\n";
//                     cout << "\n";
//                 }
// 
//                 prm::destination_list.push_back(content_wo_prefix);
//                 prm::is_sync_mode_list.push_back(false);
// 
//             } else if (first_character == '<' && second_character == '>') {
// 
//                 if (is_show_mode) {
//                     cout << ynn::fg_blue_bright << p_source_path_tmp << ynn::color_end << "\n";
//                     cout << "\n";
//                 }
// 
//                 prm::destination_list.push_back(&(p_source_path_tmp[3]));
//                 prm::is_sync_mode_list.push_back(true);
// 
//             } else if (first_character == ' ') {
// 
//                 cout << ynn::fg_red_bright << "WARNING:" << ynn::color_end << " [ " << p_source_path_tmp << " ]は空白から始まっています。\n";
//                 cout << "\n";
// 
//             } else if (first_character == '/') { //バックアップ元を指定する行のとき(完全パスでバックアップ元を指定することを前提とする)
// 
//                 source_list = source_list + p_source_path_tmp + " ";
// 
//             } else if (first_character == '#' || first_character == '\0') {
// 
//                 continue;
// 
//             } else {
// 
//                 cout << "行[ " << p_source_path_tmp << " ]の解析に失敗しました。\n";
//                 return 1;
// 
//             }
// 
//         }
// 
//         ifs.close();
// 
// //         if (is_show_mode) {
// //             output_indent();
// //             --recursion_level;
// //             cout << "設定ファイル[ " << configuration_file_name << " ]の読み取りを完了しました。\n";
// //         }
// 
//         return 0;
// 
//     };
// 
//     //source_list_file.size()の値は、--bkrcオプションが無い限りは1
//     for (int j = 0; j < source_list_file.size(); ++j) {
// 
//         int exit_status = parse_configuration_file(source_list_file[j].c_str());
// 
//         if (exit_status != 0) {
//             return 1;
//         }
// 
//     }
// 
//     //バックアップ元の表示
//     if (is_show_mode) {
//         cout << ynn::fg_blue_bright << "#--- show mode ---#" << ynn::color_end << "\n\n";
//         cout << "[バックアップ元]\n";
//         cout << source_list << "\n\n";
//     } else {
//         cout << "\n";
//     }
// 
//     string command_str;
// 
//     //prm::destination_listの内容を変える場合には、prm::is_sync_mode_listの内容も同時に変えなければならない
//     //今の実装では、両者の長さが揃っていることを要請している
//     assert(prm::destination_list.size() == prm::is_sync_mode_list.size());
// 
//     for (int i = 0; i < prm::destination_list.size(); ++i) {
// 
//         //バックアップ先が存在するかの確認
//         if (!is_show_mode) {
// 
//             wordexp_t resolved_path_structure;
//             wordexp(prm::destination_list[i].c_str(), &resolved_path_structure, 0);
//             const string resolved_path = resolved_path_structure.we_wordv[0];
//             wordfree(&resolved_path_structure);
// 
//             if (resolved_path.find('@') == string::npos) { //バックアップ先がローカルであるとき(リモートである場合は、無条件に存在と見なす)
//                 if (realpath(resolved_path.c_str(), NULL) == NULL) {
//                     cout << ynn::fg_red_bright << "[ " << resolved_path << " ]は存在しません。スキップします。" << ynn::color_end << "\n";
//                     continue;
//                 }
//             }
// 
//         }
// 
//         if (prm::is_sync_mode_list[i]) {
//             command_str = prm::base_command_when_sync_mode;
//         } else {
//             command_str = prm::base_command;
//         }
// 
//         //rsyncへ--dry-runオプションを渡している場合には、転送後の統計を出力する
//         //これを行わなければ、dry-runであるかどうかを視覚的に知ることができない
//         if (i == 0 && prm::option_to_rsync.find("--dry-run") != string::npos) {
//             prm::option_to_rsync += "--info=stats ";
//         }
// 
//         command_str += prm::option_to_rsync + source_list + prm::destination_list[i];
// 
//         cout << ynn::fg_blue_bright << "-> " << prm::destination_list[i] << ynn::color_end << "\n";
// 
//         if (is_show_mode) {
//             cout << command_str << "\n";
//         } else {
//             int exit_status = system(command_str.c_str());
//             if (exit_status != 0) {
//                 return 1;
//             }
//         }
// 
//         if (!(i == prm::destination_list.size() - 1)) {
//             cout << "\n";
//         }
// 
//     }

}

