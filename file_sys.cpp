// $Id: file_sys.cpp,v 1.6 2018-06-27 14:44:57-07 - - $

#include <iostream>
#include <stdexcept>
#include <unordered_map>

using namespace std;

#include "debug.h"
#include "file_sys.h"

int inode::next_inode_nr{ 1 };

struct file_type_hash {
    size_t operator()(file_type type) const
    {
        return static_cast<size_t>(type);
    }
};

ostream& operator<<(ostream& out, file_type type)
{
    static unordered_map<file_type, string, file_type_hash> hash{
        { file_type::PLAIN_TYPE, "PLAIN_TYPE" },
        { file_type::DIRECTORY_TYPE, "DIRECTORY_TYPE" },
    };
    return out << hash[type];
}

inode_state::inode_state()
{
    DEBUGF('i', "root = " << root << ", cwd = " << cwd 
        << ", prompt = \"" << prompt() << "\"");
    root = make_shared<inode>(file_type::DIRECTORY_TYPE);
    cwd = root;
    cwd->contents->mkdir("/", *this);

    DEBUGF('i', "root = " << root << ", cwd = " << cwd 
        << ", prompt = \"" << prompt() << "\"");
}

const string& inode_state::prompt() const { return prompt_; }

void inode_state::set_prompt(string prompt)
{
    prompt_ = prompt;
}

inode_ptr inode_state::get_cwd()
{
    return cwd;
}
inode_ptr inode_state::get_root()
{
    return root;
}
void inode_state::set_path(vector<string> new_path)
{
    dir_vec = new_path;
}

vector<string> inode_state::get_path()
{
    return dir_vec;
}
void inode_state::set_cwd(inode_ptr new_cwd)
{
    cwd = new_cwd;
}

ostream& operator<<(ostream& out, const inode_state& state)
{
    out << "inode_state: root = " << state.root
        << ", cwd = " << state.cwd;
    return out;
}

inode::inode(file_type type)
    : inode_nr(next_inode_nr++)
{
    switch (type) {
    case file_type::PLAIN_TYPE:
        contents = make_shared<plain_file>();
        break;
    case file_type::DIRECTORY_TYPE:
        contents = make_shared<directory>();
        break;
    }
    DEBUGF('i', "inode " << inode_nr << ", type = " << type);
}

int inode::get_inode_nr() const
{
    DEBUGF('i', "inode = " << inode_nr);
    return inode_nr;
}

base_file_ptr inode::get_contents()
{
    return contents;
}

file_error::file_error(const string& what)
    : runtime_error(what)
{
}

size_t plain_file::size() const
{
    size_t size{ 0 };

    if (data.size() == 0) {
        return size;
    }
    for (auto str : data) {
        size += str.size() + 1;
    }
    --size;
    DEBUGF('i', "size = " << size);
    return size;
}

const wordvec& plain_file::readfile() const
{
    DEBUGF('i', data);
    return data;
}

void plain_file::writefile(const wordvec& words)
{
    DEBUGF('i', words);
    data = words;
}

void plain_file::remove(const string&)
{
    throw file_error("is a plain file");
}

inode_ptr plain_file::mkdir(const string&, inode_state& state)
{
    DEBUGF('i', state);
    throw file_error("is a plain file");
}

inode_ptr plain_file::mkfile(const string&, inode_state&)
{
    throw file_error("is a plain file");
}

map<string, inode_ptr> plain_file::get_map()
{
    throw file_error("is a plain file");
}

bool plain_file::is_directory()
{
    return false;
}

inode_ptr plain_file::get_parent()
{
    return parent;
}

void plain_file::set_map(map<string, inode_ptr> new_map)
{
    DEBUGF('i', new_map.begin()->first);
    throw file_error("is a plain file");
}

bool plain_file::is_empty()
{
    return size() == 0;
}

size_t directory::size() const
{
    size_t size = dirents.size();
    DEBUGF('i', "size = " << size);
    return size;
}

const wordvec& directory::readfile() const
{
    throw file_error("is a directory");
}

void directory::writefile(const wordvec&)
{
    throw file_error("is a directory");
}

void directory::remove(const string& filename)
{
    DEBUGF('i', filename);
    if (dirents[filename]->get_contents()->is_directory()) {
        if (dirents[filename]->get_contents()->size() != 2) {
            throw file_error("directory is not empty");
        }
    }
    dirents.erase(filename);
}
bool directory::is_directory()
{
    return true;
}

inode_ptr directory::mkdir(const string& dirname, inode_state& state)
{
    DEBUGF('i', dirname);
    inode_ptr self;

    if (dirname == "/") {
        dirents.insert(pair<string, inode_ptr>("..",
            state.get_cwd()));
        dirents.insert(pair<string, inode_ptr>(".", 
            state.get_cwd()));
        self = state.get_cwd();
        self->get_contents()->set_parent(self);

    } else {
        self = make_shared<inode>(file_type::DIRECTORY_TYPE);

        DEBUGF('i', dirname);
        dirents[dirname] = self;
        map<string, inode_ptr> new_map;
        new_map.insert(pair<string, inode_ptr>("..", 
            state.get_cwd()));
        new_map.insert(pair<string, inode_ptr>(".", self));
        self->get_contents()->set_map(new_map);
        self->get_contents()->set_parent(state.get_cwd());
    }
    return self;
}

inode_ptr directory::mkfile(const string& filename, 
    inode_state& state)
{
    DEBUGF('i', filename);
    inode_ptr file = make_shared<inode>(file_type::PLAIN_TYPE);
    dirents.insert(pair<string, inode_ptr>(filename, file));
    file->get_contents()->set_parent(state.get_cwd());
    return file;
}

map<string, inode_ptr> directory::get_map()
{
    return dirents;
}

inode_ptr directory::get_parent()
{
    return parent;
}
void plain_file::set_parent(const inode_ptr& in)
{
    parent = in;
}

void directory::set_parent(const inode_ptr& in)
{
    parent = in;
}

void directory::set_map(map<string, inode_ptr> new_map)
{
    dirents = new_map;
}

bool directory::is_empty()
{
    return size() <= 2;
}
