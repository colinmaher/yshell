// $Id: commands.cpp,v 1.17 2018-01-25 14:02:55-08 - - $

#include "commands.h"
#include "debug.h"

command_hash cmd_hash{
    { "cat", fn_cat },
    { "cd", fn_cd },
    { "echo", fn_echo },
    { "exit", fn_exit },
    { "ls", fn_ls },
    { "lsr", fn_lsr },
    { "make", fn_make },
    { "mkdir", fn_mkdir },
    { "prompt", fn_prompt },
    { "pwd", fn_pwd },
    { "rm", fn_rm },
    {"rmr", fn_rmr },
};

command_fn find_command_fn(const string& cmd)
{
    // Note: value_type is pair<const key_type, mapped_type>
    // So: iterator->first is key_type (string)
    // So: iterator->second is mapped_type (command_fn)
    DEBUGF('c', "[" << cmd << "]");
    const auto result = cmd_hash.find(cmd);
    if (result == cmd_hash.end()) {
        throw command_error(cmd + ": no such function");
    }
    return result->second;
}

command_error::command_error(const string& what)
    : runtime_error(what)
{
}

int exit_status_message()
{
    int exit_status = exit_status::get();
    cout << execname() << ": exit(" << exit_status << ")" << endl;
    return exit_status;
}

void format(pair<string, inode_ptr> p)
{
    unsigned int nr_digits = 0;
    unsigned int size_digits = 0;
    unsigned int m = p.second->get_inode_nr();
    while (m != 0) {
        m = m / 10;
        ++nr_digits;
    }
    unsigned int l = p.second->get_contents()->size();

    if (l == 0) {
        ++size_digits;
    }
    while (l != 0) {
        l = l / 10;
        ++size_digits;
    }
    for (unsigned int i = 0; i < 6 - nr_digits; ++i) {
        cout << " ";
    }
    cout << p.second->get_inode_nr();
    for (unsigned int i = 0; i < 7 - size_digits; ++i) {
        cout << " ";
    }

    cout << p.second->get_contents()->size();
    cout << "  " << p.first;

    p.second->get_contents()->is_directory() && 
    p.first != "." && p.first != ".." ? 
    cout << "/" << endl : cout << endl;
}

vector<string> get_tokens(string path)
{
    char* cptr;
    char* cstr = new char[path.length() + 1];
    strcpy(cstr, path.c_str());
    cptr = strtok(cstr, "/");
    vector<string> tokens;

    while (cptr != NULL) {
        tokens.push_back(cptr);
        cptr = strtok(NULL, "/");
    }
    return tokens;
}

inode_ptr resolve_path(vector<string> tokens, inode_state& state,
    bool root)
{
    inode_ptr current;

    if (root) {
        current = state.get_root();
    } else {
        current = state.get_cwd();
    }

    map<string, inode_ptr> current_map = 
    current->get_contents()->get_map();

    unsigned int token_count = 1;
    for (auto token : tokens) {
        if (current_map.find(token) != current_map.end() && 
        current_map[token]->get_contents()->is_directory()) {
            current = current_map[token];
            if (tokens.size() == token_count) {
                return current;
            }
            current_map = current->get_contents()->get_map();
            ++token_count;

        } else if (current_map.find(token) != current_map.end()
            && tokens.size() == token_count) {
            return current;
        } else {
            return nullptr;
        }
    }
    return nullptr;
}

void fn_cat(inode_state& state, const wordvec& words)
{
    DEBUGF('c', state);
    DEBUGF('c', words);
    if (words.size() == 1) {
        throw command_error("cat: pathname required");
    }
    wordvec files(words.begin() + 1, words.end());
    vector<string> tokens;
    inode_ptr current;
    map<string, inode_ptr> dir;
    bool is_root = words[1][0] == '/' || 
    state.get_cwd() == state.get_root() ? true : false;
    DEBUGF('c', files);
    for (auto file : files) {
        tokens = get_tokens(file);
        vector<string> path(tokens.begin(), tokens.end() - 1);
        if (tokens.size() != 1) {
            current = resolve_path(path, state, is_root);
        } else {
            current = state.get_cwd();
        }
        if (current != nullptr && current->get_contents()
        ->is_directory()) {
            dir = current->get_contents()->get_map();
            if(dir.find(tokens[tokens.size() - 1]) != dir.end()){
                cout << dir[tokens[tokens.size() - 1]]
                ->get_contents()
                ->readfile()
                << endl;
            }else{
                throw command_error("cat: No such file");
            }
           
        } else {
            throw command_error("cat: No such file");
        }
    }
}

void fn_cd(inode_state& state, const wordvec& words)
{
    DEBUGF('c', state);
    DEBUGF('c', words);

    if (words.size() == 1) {
        throw command_error("cd: pathname required");
    }

    inode_ptr current;
    vector<string> input_tokens = get_tokens(words[1]);
    vector<string> current_path = state.get_path();
    if (words[1][0] == '/') {
        current = state.get_root();
    } else {
        current = state.get_cwd();
    }
    map<string, inode_ptr> dir;
    for (auto token : input_tokens) {
        dir = current->get_contents()->get_map();
        if (dir.find(token) != dir.end()) {
            if (token == "..") {
                current_path.pop_back();
            } else {
                current_path.push_back(token);
            }
            current = dir[token];
        } else {
            throw command_error("cd: No such file or directory");
        }
    }
    state.set_cwd(current);
    if (current == state.get_root()) {
        state.set_path(wordvec{});
    } else {
        state.set_path(current_path);
    }
}

void fn_echo(inode_state& state, const wordvec& words)
{
    DEBUGF('c', state);
    DEBUGF('c', words);
    cout << word_range(words.cbegin() + 1, words.cend()) << endl;
}

void fn_exit(inode_state& state, const wordvec& words)
{
    DEBUGF('c', state);
    DEBUGF('c', words);
    int error_code;
    if (words.size() > 1) {
        try {
            error_code = stoi(words[1]);
        } catch (const invalid_argument& ia) {
            error_code = 127;
        }
        exit_status::set(error_code);
    }

    throw ysh_exit();
}

void fn_ls(inode_state& state, const wordvec& words)
{
    DEBUGF('c', state);
    DEBUGF('c', words);

    if (words.size() == 1) {
        fn_pwd(state, words);
        for (auto p : state.get_cwd()->get_contents()->get_map()) {
            format(p);
        }

    } else {
        inode_ptr current;
        vector<string> input_tokens = get_tokens(words[1]);
        vector<string> initial_path = state.get_path();
        vector<string> current_path = initial_path;
        if (words[1][0] == '/') {
            current = state.get_root();
        } else {
            current = state.get_cwd();
        }
        inode_ptr initial_dir = state.get_cwd();
        map<string, inode_ptr> dir;
        for (auto token : input_tokens) {
            dir = current->get_contents()->get_map();
            if (dir.find(token) != dir.end()) {
                if (token == "..") {
                    current_path.pop_back();
                } else {
                    current_path.push_back(token);
                }
                current = dir[token];
            } else {
                throw command_error("cd: No such file or directory");
            }
        }
        state.set_cwd(current);
        state.set_path(current_path);
        fn_pwd(state, words);
        state.set_cwd(initial_dir);
        state.set_path(initial_path);

        for (auto p : current->get_contents()->get_map()) {
            format(p);
        }
    }
}

void lsr_helper(inode_state& state, const wordvec& words)
{
    fn_pwd(state, words);
    for (auto pair : state.get_cwd()->get_contents()->get_map()) {
        format(pair);
    }
    vector<string> initial_path = state.get_path();
    vector<string> current_path = initial_path;
    inode_ptr initial_dir = state.get_cwd();
    for (auto p : state.get_cwd()->get_contents()->get_map()) {
        if (p.second->get_contents()
        ->is_directory() && p.first != "."
            && p.first != "..") {
            current_path.push_back(p.first);
            state.set_cwd(p.second);
            state.set_path(current_path);
            lsr_helper(state, words);
            current_path.pop_back();
            state.set_path(initial_path);
            state.set_cwd(initial_dir);
        }
    }
}

void fn_lsr(inode_state& state, const wordvec& words)
{
    DEBUGF('c', state);
    DEBUGF('c', words);
    vector<string> initial_path = state.get_path();
    vector<string> current_path = initial_path;
    inode_ptr initial_dir = state.get_cwd();
    if (words.size() == 1 || words[1] == "/") {
        state.set_cwd(state.get_root());
        state.set_path(wordvec{});
        lsr_helper(state, words);
        state.set_path(initial_path);
        state.set_cwd(initial_dir);
    } else {
        inode_ptr current;
        vector<string> input_tokens = get_tokens(words[1]);
        if(input_tokens.size() == 0){
            lsr_helper(state, words);
        }



        if (words[1][0] == '/') {
            current = state.get_root();
        } else {
            current = state.get_cwd();
        }

        map<string, inode_ptr> dir;
        for (auto token : input_tokens) {
            dir = current->get_contents()->get_map();
            if (dir.find(token) != dir.end() && 
            current->get_contents()
            ->is_directory()) {
                if (token == "..") {
                    current_path.pop_back();
                } else {
                    current_path.push_back(token);
                }
                current = dir[token];
            } else {
                throw command_error("cd: No such file or directory");
            }
        }
        state.set_cwd(current);
        state.set_path(current_path);
        lsr_helper(state, words);
        state.set_path(initial_path);
        state.set_cwd(initial_dir);
    }
}

void fn_make(inode_state& state, const wordvec& words)
{
    DEBUGF('c', state);
    DEBUGF('c', words);

    if (words.size() == 1) {
        throw command_error("filename required");
    }
    map<string, inode_ptr> dir;
    vector<string> tokens = get_tokens(words[1]);
    vector<string> path(tokens.begin(), tokens.end() - 1);
    inode_ptr current = state.get_cwd();
    bool is_root = words[1][0] == '/' || 
    state.get_cwd() == state.get_root() ? true : false;

    if (tokens.size() != 1) {
        current = resolve_path(path, state, is_root);
    }
    if (current != nullptr) {
        if (current->get_contents()->is_directory()) {
            dir = current->get_contents()->get_map();
            if (dir.find(tokens[tokens.size() - 1]) != dir.end()) {
                if (words.size() == 2) {
                    throw command_error("make: file already exists");
                }
                wordvec new_data(words.begin() + 2, words.end());
                dir[tokens[tokens.size() - 1]]
                ->get_contents()->writefile(new_data);
            } else {
                inode_ptr file = current->get_contents()
                ->mkfile(tokens[tokens.size() - 1], state);
                current = file;
                if (words.size() > 2) {
                    wordvec new_data(words.begin() + 2, words.end());
                    current->get_contents()->writefile(new_data);
                }
            }
        } else {
            if (words.size() == 2) {
                throw command_error("make: file already exists");
            }
            wordvec new_data(words.begin() + 2, words.end());
            current->get_contents()->get_parent()
            ->get_contents()->writefile(new_data);
        }
    } else {
        throw command_error("make: No such file or directory");
    }
}

void fn_mkdir(inode_state& state, const wordvec& words)
{
    DEBUGF('c', state);
    DEBUGF('c', words);

    if (words.size() == 1) {
        throw command_error("mkdir: pathname required");
    }
    vector<string> tokens;
    map<string, inode_ptr> dir;
    tokens = get_tokens(words[1]);
    bool is_root = words[1][0] == '/' || 
    state.get_cwd() == state.get_root() ? true : false;
    vector<string> path(tokens.begin(), tokens.end() - 1);
    // cout << "lookup path:" << path << endl;
    // inode_ptr initial_cwd = state.get_cwd();
    inode_ptr current = resolve_path(path, state, is_root);
    DEBUGF('c', state.get_root());
    DEBUGF('c', current);
    if (current != nullptr) {
        dir = current->get_contents()->get_map();
        // cout << tokens[tokens.size() - 1] << endl;
        if (!current->get_contents()->is_directory() || 
        dir.find(tokens[tokens.size() - 1]) != dir.end()) {
            throw command_error("mkdir: path already exists");
        } else {
            // state.set_cwd(current);

            current->get_contents()
                ->mkdir(tokens[tokens.size() - 1], state);
            // state.set_cwd(initial_cwd);
        }
    } else if (tokens.size() == 1) {
        current = state.get_cwd();
        current->get_contents()
            ->mkdir(tokens[tokens.size() - 1], state);
    } else {
        throw command_error("mkdir: No such file or directory");
    }
}

void fn_prompt(inode_state& state, const wordvec& words)
{
    DEBUGF('c', state);
    DEBUGF('c', words);
    string prompt = "";
    for (auto it = words.cbegin() + 1; it != words.cend(); ++it) {
        prompt += *it;
        prompt += " ";
    }
    state.set_prompt(prompt);
}

void fn_pwd(inode_state& state, const wordvec& words)
{
    DEBUGF('c', state);
    DEBUGF('c', words);
    string path = "/";
    for (auto token : state.get_path()) {
        path += token + "/";
    }
    if (path != "/") {
        path.pop_back();
    }
    cout << path + ":" << endl;
}

void fn_rm(inode_state& state, const wordvec& words)
{
    DEBUGF('c', state);
    DEBUGF('c', words);
    if (words.size() == 1) {
        throw command_error("rm: pathname required");
    }
    wordvec files(words.begin() + 1, words.end());
    vector<string> tokens;
    bool is_root = words[1][0] == '/' || 
    state.get_cwd() == state.get_root() ? true : false;
    DEBUGF('c', files);
    for (auto file : files) {
        tokens = get_tokens(file);
        vector<string> path(tokens.begin(), tokens.end() - 1);
        inode_ptr current = resolve_path(path, state, is_root);
        if (current != nullptr && 
        current->get_contents()->is_empty()) {
            current->get_contents()->remove(tokens[tokens.size() - 1]);
        } else if (tokens.size() == 1
            && current->get_contents()->is_empty()) {
            current = state.get_cwd();
            current->get_contents()->remove(tokens[tokens.size() - 1]);
        } else {
            throw command_error("rm: No such file or directory");
        }
    }
}

void rmr_helper(inode_state& state, const wordvec& words)
{
    // fn_pwd(state, words);
    // for (auto pair : state.get_cwd()->get_contents()->get_map()) {
    //     format(pair);
    // }
    
    // vector<string> initial_path = state.get_path();
    // vector<string> current_path = initial_path;
    inode_ptr initial_dir = state.get_cwd();
    for (auto p : state.get_cwd()->get_contents()->get_map()) {
        if (p.second->get_contents()->is_directory() && p.first != "."
            && p.first != "..") {
            // current_path.push_back(p.first);
            state.set_cwd(p.second);
            // state.set_path(current_path);
            rmr_helper(state, words);
            if(state.get_cwd()->get_contents()->is_empty()){
                for(auto p : state.get_cwd()->get_contents()->get_map()){
                    if (p.first != "."
                    && p.first != "..") {
                        state.get_cwd()->get_contents()->get_parent()->get_contents()->get_map().erase(p.first);
                        cout << "erased " << endl;
                    }
                    map<string, inode_ptr> new_map = {};
                    state.get_cwd()->get_contents()->set_map(new_map);
                }
            }
            else{
                throw command_error("rmr: directory not empty");
            }
            // current_path.pop_back();
            // state.set_path(initial_path);
            state.set_cwd(initial_dir);
        }
    }
    


}

void fn_rmr(inode_state& state, const wordvec& words)
{
    DEBUGF('c', state);
    DEBUGF('c', words);
    if (words.size() == 1 || words[1] == "/") {
        // cout << "here" << endl;
        rmr_helper(state, words);
    } else {
        inode_ptr current;
        vector<string> input_tokens = get_tokens(words[1]);
        // vector<string> initial_path = state.get_path();
        // vector<string> current_path = initial_path;
        inode_ptr initial_dir = state.get_cwd();

        if (words[1][0] == '/') {
            current = state.get_root();
        } else {
            current = state.get_cwd();
        }

        map<string, inode_ptr> dir;
        for (auto token : input_tokens) {
            dir = current->get_contents()->get_map();
            if (dir.find(token) != dir.end() && 
            current->get_contents()->is_directory()) {
                // if (token == "..") {
                //     current_path.pop_back();
                // } else {
                //     current_path.push_back(token);
                // }
                current = dir[token];
            } else {
                throw command_error("cd: No such file or directory");
            }
        }
        state.set_cwd(current);
        // state.set_path(current_path);
        rmr_helper(state, words);
        // state.set_path(initial_path);
        state.set_cwd(initial_dir);
    }
}
