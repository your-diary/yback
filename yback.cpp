#include <iostream>
using namespace std;
#include <fstream>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <set>
#include <functional>
#include <wordexp.h>
#include <cassert>

namespace prm {

    const unsigned path_length = 1000; //各バックアップ元のパスの最大長

    const string source_list_file_parent_dir = "/home/ynn/Scripts/";
    const string source_list_file_prefix = "back_";
    const string source_list_file_extension = ".txt";

    vector<string> destination_list;
    vector<bool> is_sync_mode_list; //i番目のバックアップ先に同期モードを対応させるか(同期モードに対応するとき、-a -uオプションの代わりに-a --deleteオプションを用いる。つまり、バックアップ元とバックアップ先と同一内容とする)

    const string tmp_directory_path = "/home/ynn/tmp";

    const string base_command_wo_backup = "rsync -au --info=name --log-file=\"${RSYNC_LOG}\" ";
    const string base_command = base_command_wo_backup + "--backup ";

    const string base_command_when_sync_mode = "rsync -a --delete --info=name --log-file=\"${RSYNC_LOG}\" ";

    string option_to_rsync = "";

    //--bkrcは設定ファイル中の"+ <file name>"コマンドと同義
    //--fileは設定ファイル中の"% <file name>"コマンドと同義
    const string usage_message = "\
\e[;100m用法\e[0m\n\
  back [<option(s)>] [<rsync option(s)>]\n\
  \n\
\e[;100mオプション\e[0m\n\
    \e[94m--show,-s\e[0m: コマンドを実際には実行しない(dry-run)\n\
    \e[94m--edit,-e\e[0m: 設定ファイルを編集し、バックアップを実行する\n\
    \e[94m--view,-v\e[0m: 設定ファイルを閲覧し、バックアップは実行しない\n\
     \e[94m--all,-a\e[0m: フルバックアップを実行する\n\
     \e[94m--tmp,-t\e[0m: 一時バックアップを実行する\n\
  \e[94m--with-mail\e[0m: メールのバックアップを実行する\n\
    \e[94m--file,-f\e[0m: バックアップの実行前に、続くファイル引数を一時フォルダへ複製する\n\
    \e[94m    --add\e[0m: 自動管理ファイルリストに、続くファイル引数を追加する\n\
   \e[94m--only-add\e[0m: 同上だが、実際のバックアップまでは行わない\n\
    \e[94m--bkrc,-b\e[0m: デフォルトの設定ファイルに加え、続くファイル引数も読む\n\
 \e[94m--dry-run,-n\e[0m: rsyncに--dry-runオプションを渡す\n\
    \e[94m--help,-h\e[0m: ヘルプを表示する\n\
    \e[94m       --\e[0m: 次以降の引数を強制的にrsyncオプションとする\
";

    const string color_red = "\e[91m";
    const string color_blue = "\e[94m";
    const string color_end = "\e[0m";

}

template <typename Type>
void print(const Type &input) {
    cout << input << "\n";
}

string generate_source_list_filename(const string &option) {
    return prm::source_list_file_parent_dir + prm::source_list_file_prefix + option + prm::source_list_file_extension;
}

void print_error(const string &option_string) {
    cout << "オプション[ " << option_string << " ]の解析中にエラーが発生しました。\n";
}

void print_file_not_exist_error(const string &filename) {
    cout << "ファイル[ " << filename << " ]の読み取りに失敗しました。\n";
}

bool is_file_exist(const char *file_path) {
    char *absolute_path = realpath(file_path, NULL);
    return (absolute_path != NULL);
}

int main(int argc, char **argv) {

    vector<string> source_list_file; //バックアップ元の一覧を書いた設定ファイルの名前の配列
                                     //--bkrcオプションが指定されなければ、要素数は1
    source_list_file.push_back(generate_source_list_filename("normal"));

    vector<string> should_copied_file_list; //--fileオプションで使用
    vector<string> should_added_file_list; //--addオプションで使用

    bool is_show_mode = false; //--show,-s
    bool is_edit_mode = false; //--edit,-e
    bool is_view_mode = false; //--view,-v
    bool is_only_add_mode = false; //--only-add

    unsigned first_index_of_option_to_rsync = 1;

    string option;
    bool parse_error_flag = false;

    //オプションの解析
    //汎用性の高いオプションパーサとなっているため、別のプログラムに流用できる
    for (int i = 1; i < argc; ++i) {

        option = argv[i];

        unsigned num_word_of_this_option = 1; //現在のオプションの構成ワード数
                                              //例えば--helpの構成ワード数は1
                                              //例えば--file <file>の構成ワード数は2

        if (option == "--help" || option == "-h") {
            print(prm::usage_message);
            return 0;
        } else if (option == "--show" || option == "-s") {
            is_show_mode = true;
        } else if (option == "--edit" || option == "-e") {
            is_edit_mode = true;
            is_show_mode = true;
        } else if (option == "--view" || option == "-v") {
            is_view_mode = true;
            is_show_mode = true;
        } else if (option == "--all" || option == "-a") {
            source_list_file.erase(source_list_file.begin());
            source_list_file.push_back(generate_source_list_filename("all"));
        } else if (option == "--tmp" || option == "-t") {
            source_list_file.erase(source_list_file.begin());
            source_list_file.push_back(generate_source_list_filename("tmp"));
        } else if (option == "--with-mail") {
            source_list_file.erase(source_list_file.begin());
            source_list_file.push_back(generate_source_list_filename("with_mail"));
        } else if (option == "--file" || option == "-f") {
            num_word_of_this_option = 2;
            if (i < argc - 1) {
                if (argv[i + 1][0] == '-') { //ファイル名がハイフンから始まっていれば
                    parse_error_flag = true;
                } else {
                    should_copied_file_list.push_back(argv[i + 1]);
                }
            } else {
                parse_error_flag = true;
            }
        } else if (option == "--add" || option == "--only-add") { //自動管理リストにファイルを追加するオプション(自動管理については、実装部のコメントを参照)
            if (option == "--only-add") {
                is_only_add_mode = true;
            }
            num_word_of_this_option = 2;
            if (i < argc - 1) {
                if (argv[i + 1][0] == '-') { //ファイル名がハイフンから始まっていれば
                    parse_error_flag = true;
                } else {
                    char *absolute_path = realpath(argv[i + 1], NULL); //絶対パスの取得と存在確認
                    if (absolute_path == NULL) {
                        cout << "ファイル[ " << argv[i + 1] << " ]が存在しません。\n";
                        parse_error_flag = true;
                    } else {
                        should_added_file_list.push_back(absolute_path);
                    }
                }
            } else {
                parse_error_flag = true;
            }
        } else if (option == "--bkrc" || option == "-b") {
            num_word_of_this_option = 2;
            if (i < argc - 1) {
                if (argv[i + 1][0] == '-') {
                    parse_error_flag = true;
                }
                source_list_file.push_back(argv[i + 1]);
            } else {
                parse_error_flag = true;
            }
        } else if (option == "--dry-run" || option == "-n") {
            prm::option_to_rsync = "--dry-run "; //最後のスペースに注意
        } else if (option == "--") {
            ++first_index_of_option_to_rsync;
            break;
        } else {
//             break; //不明なオプションはrsyncに渡す
            parse_error_flag = true; //不明なオプションはエラーとする(より安全)
        }

        if (parse_error_flag) {
            print_error(option);
            return 1;
        }

        first_index_of_option_to_rsync += num_word_of_this_option;
        i += (num_word_of_this_option - 1);

    }

    //rsyncへ受け渡すオプションの解析
    for (int i = first_index_of_option_to_rsync; i < argc; ++i) {
        prm::option_to_rsync += argv[i] + string(" ");
    }

    if (is_view_mode) {

        const string edit_command = "$EDITOR -R -c 'set nomodifiable' " + source_list_file[0];
        cout << prm::color_blue << "設定ファイル[ " << source_list_file[0] << " ]を確認します。" << prm::color_end << "\n";
        system(edit_command.c_str());
        cout << prm::color_blue << "確認を完了しました。" << prm::color_end << "\n\n";

    } else if (is_edit_mode) {

        const string edit_command = "$EDITOR " + source_list_file[0];
        cout << prm::color_blue << "設定ファイル[ " << source_list_file[0] << " ]を編集します。" << prm::color_end << "\n";
        system(edit_command.c_str());
        cout << prm::color_blue << "編集を完了しました。" << prm::color_end << "\n\n";

    }

    //--fileオプション用の処理(設定ファイル中の%コマンドの処理と殆ど同じだが、こちらはクォーティングもしなければならない)
    for (int j = 0; j < should_copied_file_list.size(); ++j) {
        const string command_tmp = prm::base_command_wo_backup + '"' + should_copied_file_list[j] + "\" " + prm::tmp_directory_path;
        cout << prm::color_blue << "% [" << should_copied_file_list[j] << " -> " << prm::tmp_directory_path << "]" << prm::color_end << "\n";
        if (!is_show_mode) { system(command_tmp.c_str()); }
        cout << "\n";
    }

    //自動ファイル管理の処理(--addオプションの処理を含む) {
    //
    //自動ファイル管理は、-fオプションの一般化である
    //-fオプションは便利だが、毎回指定しなければならないという欠点がある
    //自動ファイル管理は、いつも-fオプションの対象に指定するファイルを定義することに近い
    //これは次のように利用できる
    //1. --addオプションを通じて対象ファイルをリスト(back_managed_file_list.txt)に追加する
    //2. back.outを実行する度に、リストが読まれ、リスト中のファイルは自動的に~/tmp以下にバックアップされる
    //これを実装するに当たって、解決しなければならない問題は、次の二点である
    //1. 自動管理の対象に設定したファイルが削除されているとき、それはリストから削除する
    //2. リスト中に同じファイルが二つ存在しないようにする(同一のファイルに対して何度--addオプションを読んでも問題が生じないようにする)
    //これを踏まえて、次のように実装している
    //1. 既存のリストの内容と、今回--addオプションによって追加されたファイルを同一の集合に読む
    //2. その際、ファイルの存在確認を行い、存在しないファイルは集合に追加しない(集合を用いているため、重複の確認は不要である)
    //3. 集合の内容をリストファイルに書き戻す
    //4. 集合の内容からバックアップコマンドを生成し、実行する

    if (is_show_mode) {
        goto end_of_managed_file;
    }

    {

        //過去の版において、リストファイルは、内容が変更されない場合にも同じ内容で上書きされていた
        //そして、これはmtimeを更新するため、back_normal.txtなどの設定によっては、リストファイルが毎回必ずバックアップ対象となってしまっていた
        //現在は、変更が生じた際にフラグを立て、そのフラグが立っている場合に限り、リストをファイルに書き戻すようにしている
        bool is_list_modified = false;

        cout << "\n" << prm::color_blue << "[自動ファイル管理]" << prm::color_end << "\n";

        //自動管理の対象のファイルを記述するファイル
        const string managed_file_list_source = generate_source_list_filename("managed_file_list");

        set<string> managed_file_set;

        //--addオプションの処理
        //should_added_file_listには絶対パスが格納されており、かつ追加対象の存在確認は済んでいる
        for (const string &s : should_added_file_list) {
            is_list_modified = true;
            cout << prm::color_blue << "ファイル[ " << s << " ]を自動管理に設定しました。"
                 << prm::color_end << "\n";
            managed_file_set.insert(s);
        }

        ifstream ifs(managed_file_list_source.c_str());
        if (!ifs) {
           cout << "ファイル[ " <<  managed_file_list_source << " ]が開けませんでした。\n";
           return 1;
        }

        const unsigned buf_size = 1000;

        while (true) {

            char buf[buf_size];

            ifs.getline(buf, buf_size);
            if (!ifs) {
                break;
            }

            //ファイルが存在しないとき、集合に追加しない(最後に集合をファイルに書き戻すため、これはリストからファイルを削除することを意味する)
            if (is_file_exist(buf)) {
                managed_file_set.insert(buf);
            } else {
                is_list_modified = true;
                cout << prm::color_red << "ファイル[ " << buf << " ]を自動管理から解放しました。"
                     << prm::color_end << "\n";
            }

        }

        ifs.close();

        if (managed_file_set.size() == 0) { //管理対象のファイルが設定されていないとき

            if (is_list_modified) {

                //リストファイルを空にする
                ofstream ofs(managed_file_list_source.c_str());
                if (!ofs) {
                   cout << "ファイル[ " <<  managed_file_list_source << " ]が開けませんでした。\n";
                   return 1;
                }
                ofs.close();

            }

            goto end_of_managed_file;

        }

        //集合の書き戻し、バックアップコマンドの生成 {

        string command_tmp = prm::base_command_wo_backup;

        if (is_list_modified) {

            ofstream ofs(managed_file_list_source.c_str());
            if (!ofs) {
               cout << "ファイル[ " <<  managed_file_list_source << " ]が開けませんでした。\n";
               return 1;
            }

            for (const string &s : managed_file_set) {
                ofs << s << "\n";
                command_tmp += '"' + s + "\" ";
            }
            command_tmp += prm::tmp_directory_path;
            ofs.close();

        } else {

            for (const string &s : managed_file_set) {
                command_tmp += '"' + s + "\" ";
            }
            command_tmp += prm::tmp_directory_path;

        }

        //} 集合の書き戻し、バックアップコマンドの生成

        system(command_tmp.c_str());
//         cout << "\n";

        if (is_only_add_mode) {
            return 0;
        }

    }
    end_of_managed_file:

    //} 自動ファイル管理の処理(--addオプションの処理を含む)

    char source_path_tmp[prm::path_length];
    char *p_source_path_tmp = source_path_tmp;

    string source_list; //クォーティングしたバックアップ元をスペース区切りで並べたもの

    //デバッグ用
    //再帰呼び出しの構造を分かりやすくするために、出力にインデントを加える
    unsigned recursion_level = 0;
    auto output_indent = [&recursion_level]() {
        for (int k = 0; k < recursion_level; ++k) {
            cout << "\t";
        }
    };

    //設定ファイル中の「別の設定ファイルを読む」という'+'コマンドに対応する必要がある
    //過去の版では、動的なfor文を使った実装をしており、+コマンドに到達した時点で対象ファイルをキューに追加し、現在のファイルは取り敢えず読み進めるという手を取っていた(ただし、ここの「キュー」は単に構造を指しており、std::queueを意味していない)
    //この実装でも、長いこと問題は生じていなかったが、これではオプションの上書きが正常に行われないことが発覚した
    //例えば、ファイルAに「ファイルBを読む」(命令1)および「--suffix=Aを設定する」(命令2)と書かれており、ファイルBに「--suffix=Bを設定する」(命令3)と書かれている場合、命令は1 -> 2 -> 3の順番で読まれ、最終的に有効となるのは命令3である
    //しかしながら、ファイルAの意図としては、ファイルBの設定に対する追加または削除を行いたいのであるから、命令は1 -> 3 -> 2の順番で読まれ、最終的に命令2が有効化されるのが自然である
    //これを実現するためには、+コマンドに達した時点で即座に対象ファイルの読み取りを始め、それが終わったら、また読み取り前の位置に戻ってくれば良い
    //これには再帰関数を用いるのが簡単であるため、再帰的なラムダ式を採用することにした
    //なお、もはやsource_list_fileをベクトルで保持する必要は本質的には無いが、--bkrcオプションがあるため、単なる文字列に変えてしまうわけにもいかない(例えば、source_list_fileを単なる文字列にして、--bkrcオプションの引数は別の文字列で保持し、後者が空でなければそちらに対してもparse_configuration_file()を呼び出す、などとすれば、ベクトルを廃止できる)

    function<int (const char *configuration_file_name)> parse_configuration_file = [&](const char *configuration_file_name) -> /* exit status */ int {

//         if (is_show_mode) {
//             output_indent();
//             cout << "設定ファイル[ " << configuration_file_name << " ]の読み取りを開始しました。\n";
//         }

        ifstream ifs(configuration_file_name);
        if (!ifs) {
            print_file_not_exist_error(configuration_file_name);
            return 1;
        }

        while (true) {

            ifs.getline(p_source_path_tmp, prm::path_length - 1);
            if (!ifs) {
                break;
            }

            const char first_character = p_source_path_tmp[0];
            const char second_character = p_source_path_tmp[1];
            const string content_wo_prefix = &(p_source_path_tmp[2]); //行の内容の三文字目以降

            if (first_character == '!') {

                break;

            } else if (first_character == '?') {
                
                string buf;
                cout << content_wo_prefix << "(y/n): ";
                cin >> buf;

                if (!cin) {
                    cout << "\n入力が不正です。\n";
                    return 1;
                } else if (buf[0] == 'y' && buf.size() == 1) {
                    cout << "コマンドの実行を続行します。\n";
                } else {
                    cout << "コマンドの実行をキャンセルしました。\n";
                    return 1;
                }

            } else if (first_character == '-') {

                prm::option_to_rsync += p_source_path_tmp + string(" ");

            } else if (first_character == '+') {

                cout << prm::color_blue << "+ " << content_wo_prefix << prm::color_end << "\n\n";

                if (is_show_mode) {
                    ++recursion_level;
                }

                int exit_status = parse_configuration_file(content_wo_prefix.c_str()); //再帰的呼び出し
                if (exit_status != 0) {
                    return 1;
                }

            } else if (first_character == '%') {

                const string command_tmp = prm::base_command_wo_backup + content_wo_prefix + " " + prm::tmp_directory_path;
                cout << prm::color_blue << "% [" << content_wo_prefix << " -> " << prm::tmp_directory_path << "]" << prm::color_end << "\n";
                if (!is_show_mode) { system(command_tmp.c_str()); }
                cout << "\n";

            } else if (first_character == '^') {

                if (is_show_mode) {
                    cout << prm::color_blue << "^ [" << content_wo_prefix << "]" << prm::color_end << "\n";
                    cout << "\n";
                } else {
                    const string command_tmp = prm::base_command_wo_backup + content_wo_prefix;
                    system(command_tmp.c_str());
                }

            } else if (first_character == '$') {

                cout << prm::color_blue << p_source_path_tmp << prm::color_end << "\n";
                if (!is_show_mode) { system(content_wo_prefix.c_str()); }
                cout << "\n";

            } else if (first_character == '>') {

                if (is_show_mode) {
                    cout << prm::color_blue << p_source_path_tmp << prm::color_end << "\n";
                    cout << "\n";
                }

                prm::destination_list.push_back(content_wo_prefix);
                prm::is_sync_mode_list.push_back(false);

            } else if (first_character == '<' && second_character == '>') {

                if (is_show_mode) {
                    cout << prm::color_blue << p_source_path_tmp << prm::color_end << "\n";
                    cout << "\n";
                }

                prm::destination_list.push_back(&(p_source_path_tmp[3]));
                prm::is_sync_mode_list.push_back(true);

            } else if (first_character == ' ') {

                cout << prm::color_red << "WARNING:" << prm::color_end << " [ " << p_source_path_tmp << " ]は空白から始まっています。\n";
                cout << "\n";

            } else if (first_character == '/') { //バックアップ元を指定する行のとき(完全パスでバックアップ元を指定することを前提とする)

                source_list = source_list + p_source_path_tmp + " ";

            } else if (first_character == '#' || first_character == '\0') {

                continue;

            } else {

                cout << "行[ " << p_source_path_tmp << " ]の解析に失敗しました。\n";
                return 1;

            }

        }

        ifs.close();

//         if (is_show_mode) {
//             output_indent();
//             --recursion_level;
//             cout << "設定ファイル[ " << configuration_file_name << " ]の読み取りを完了しました。\n";
//         }

        return 0;

    };

    //source_list_file.size()の値は、--bkrcオプションが無い限りは1
    for (int j = 0; j < source_list_file.size(); ++j) {

        int exit_status = parse_configuration_file(source_list_file[j].c_str());

        if (exit_status != 0) {
            return 1;
        }

    }

    //バックアップ元の表示
    if (is_show_mode) {
        cout << prm::color_blue << "#--- show mode ---#" << prm::color_end << "\n\n";
        cout << "[バックアップ元]\n";
        cout << source_list << "\n\n";
    } else {
        cout << "\n";
    }

    string command_str;

    //prm::destination_listの内容を変える場合には、prm::is_sync_mode_listの内容も同時に変えなければならない
    //今の実装では、両者の長さが揃っていることを要請している
    assert(prm::destination_list.size() == prm::is_sync_mode_list.size());

    for (int i = 0; i < prm::destination_list.size(); ++i) {

        //バックアップ先が存在するかの確認
        if (!is_show_mode) {

            wordexp_t resolved_path_structure;
            wordexp(prm::destination_list[i].c_str(), &resolved_path_structure, 0);
            const string resolved_path = resolved_path_structure.we_wordv[0];
            wordfree(&resolved_path_structure);

            if (resolved_path.find('@') == string::npos) { //バックアップ先がローカルであるとき(リモートである場合は、無条件に存在と見なす)
                if (realpath(resolved_path.c_str(), NULL) == NULL) {
                    cout << prm::color_red << "[ " << resolved_path << " ]は存在しません。スキップします。" << prm::color_end << "\n";
                    continue;
                }
            }

        }

        if (prm::is_sync_mode_list[i]) {
            command_str = prm::base_command_when_sync_mode;
        } else {
            command_str = prm::base_command;
        }

        //rsyncへ--dry-runオプションを渡している場合には、転送後の統計を出力する
        //これを行わなければ、dry-runであるかどうかを視覚的に知ることができない
        if (i == 0 && prm::option_to_rsync.find("--dry-run") != string::npos) {
            prm::option_to_rsync += "--info=stats ";
        }

        command_str += prm::option_to_rsync + source_list + prm::destination_list[i];

        cout << prm::color_blue << "-> " << prm::destination_list[i] << prm::color_end << "\n";

        if (is_show_mode) {
            cout << command_str << "\n";
        } else {
            int exit_status = system(command_str.c_str());
            if (exit_status != 0) {
                return 1;
            }
        }

        if (!(i == prm::destination_list.size() - 1)) {
            cout << "\n";
        }

    }

}

#if 0

#endif

