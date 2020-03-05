#ifndef is_misc_included

    #define is_misc_included

    #include <iostream>
    #include <set>
    #include <wordexp.h>
    #include <sys/wait.h>

    namespace misc {

        using namespace std;

        template <class Type>
        void print_value(const Type &value) {
            cout << value;
        }

        void print_value(const string &value) {
            cout << "\"" << value << "\"";
        }

        template <class RAI>
        void print_array(RAI first, RAI last) {
            if (first >= last) { //if the array is empty
                cout << "[]\n";
                return;
            }
            cout << "[";
            misc::print_value(*first);
            for (RAI iter = first + 1; iter != last; ++iter) {
                cout << ", ";
                misc::print_value(*iter);
            }
            cout << "]";
            cout << "\n";
        }

        template <class Type>
        void print_array(const vector<Type> &v) {
            misc::print_array(v.begin(), v.end());
        }

        bool does_start_with(const string &str, const string &prefix) {

            if (str.length() <= prefix.length()) {
                return false;
            } else {
                for (int i = 0; i < prefix.length(); ++i) {
                    if (str[i] != prefix[i]) {
                        return false;
                    }
                }
                return true;
            }

        }

        template <class RAI>
        bool has_duplicate_element(RAI first, RAI last, bool is_verbose_mode = false) {

            set<typename iterator_traits<RAI>::value_type> s;

            bool ret = false;

            for (RAI iter = first; iter != last; ++iter) {
                if (s.count(*iter)) {
                    ret = true;
                    if (is_verbose_mode) {
                        cout << __func__ << "(): The element [ " << *iter << " ] is duplicated.\n";
                    }
                } else {
                    s.insert(*iter);
                }
            }

            return ret;

        }

        vector<string> word_expansion(const string &str, bool should_also_print_result = false) {

            vector<string> ret;

            wordexp_t result_structure;
            const int return_value = wordexp(str.c_str(), &result_structure, WRDE_UNDEF /* Makes referencing an undefined variable an error. */);

            if (return_value != 0) {
                cout << "misc::" << __func__ << "(): An error occurred. Referencing an undefined variable may be the cause.\n";
                return ret;
            }

            for (int i = 0; i < result_structure.we_wordc; ++i) {
                if (should_also_print_result) {
                    cout << result_structure.we_wordv[i] << "\n";
                }
                ret.push_back(result_structure.we_wordv[i]);
            }

            wordfree(&result_structure);

            return ret;

        }

        bool does_file_exist(const string &path) {
            const string expanded_path = misc::word_expansion(path)[0];
            return (realpath(expanded_path.c_str(), NULL) != NULL);
        }

        bool does_file_exist_without_expansion(const string &path) {
            return (realpath(path.c_str(), NULL) != NULL);
        }

        bool is_directory(const string &path) {
            const char *pwd = get_current_dir_name();
            int exit_status = chdir(path.c_str());
            chdir(pwd);
            return (exit_status == 0);
        }

        int execute(const string &binary_path, const char * const argv[], const char * const envp[] = environ) {

            pid_t process_id = fork();

            if (process_id == 0) { //child process

                const int status = execvpe(binary_path.c_str(), const_cast<char ** const>(argv), const_cast<char ** const>(envp));

                if (status == -1) {
                    cout << "misc::" << __func__ << "(): Failed in executing the command [ " << binary_path.c_str() << " ].\n";
                }

                exit(status);

            } else {

                int status;
                waitpid(process_id, &status, 0);
                delete[] argv;
                return WEXITSTATUS(status);

            }

        }

        int exec(const vector<string> &v) {

            const unsigned argv_size = v.size() + 1;

            const char **argv = new const char * [argv_size];
            argv[argv_size - 1] = nullptr;

            for (int i = 0; i < v.size(); ++i) {
                argv[i] = v[i].c_str();
            }

            return misc::execute(argv[0], argv);

        }

    }

#endif

