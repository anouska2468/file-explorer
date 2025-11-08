/*
-------------------------------------------------------
 File Explorer Application (C++)
 Author: Anouska Rout
 Description:
   Console-based file explorer using Linux system calls.
   Features: list files, show details (permissions, size, mtime),
             create/delete files, change directory, recursive search.
-------------------------------------------------------
*/
#include <iostream>
#include <dirent.h>    // opendir, readdir, closedir
#include <sys/stat.h>  // stat, struct stat
#include <unistd.h>    // chdir, getcwd
#include <pwd.h>       // getpwuid (optional owner name)
#include <grp.h>       // getgrgid (optional group name)
#include <cstring>     // strcmp, strerror
#include <ctime>       // ctime
#include <vector>
#include <string>
#include <algorithm>   // for sort()
#include <iomanip>     // setw
#include <errno.h>

using namespace std;

// ---------- Helper: format permission bits like "rwxr-xr--" ----------
string formatPermissions(mode_t mode) {
    string perms = "----------";

    // File type
    if (S_ISDIR(mode)) perms[0] = 'd';
    else if (S_ISLNK(mode)) perms[0] = 'l';
    else perms[0] = '-';

    // Owner
    if (mode & S_IRUSR) perms[1] = 'r';
    if (mode & S_IWUSR) perms[2] = 'w';
    if (mode & S_IXUSR) perms[3] = 'x';
    // Group
    if (mode & S_IRGRP) perms[4] = 'r';
    if (mode & S_IWGRP) perms[5] = 'w';
    if (mode & S_IXGRP) perms[6] = 'x';
    // Others
    if (mode & S_IROTH) perms[7] = 'r';
    if (mode & S_IWOTH) perms[8] = 'w';
    if (mode & S_IXOTH) perms[9] = 'x';

    return perms;
}

// ---------- Show file details using stat ----------
void showFileDetails(const string &path, const string &name) {
    string full = path + "/" + name;
    struct stat st;
    if (lstat(full.c_str(), &st) != 0) {
        cerr << "  [stat error] " << name << " : " << strerror(errno) << "\n";
        return;
    }

    string perms = formatPermissions(st.st_mode);
    // owner and group names (optional)
    struct passwd *pw = getpwuid(st.st_uid);
    struct group  *gr = getgrgid(st.st_gid);
    string owner = pw ? pw->pw_name : to_string(st.st_uid);
    string group = gr ? gr->gr_name : to_string(st.st_gid);

    // modification time
    char timebuf[64];
    struct tm *tm = localtime(&st.st_mtime);
    strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", tm);

    cout << left << setw(12) << perms 
         << setw(8) << owner 
         << setw(8) << group 
         << setw(10) << st.st_size 
         << setw(20) << timebuf 
         << " " << name << "\n";
}

// ---------- List files (simple view or detailed) ----------
void listFiles(const string &path, bool detailed = false) {
    DIR *dir = opendir(path.c_str());
    if (!dir) {
        cerr << "opendir failed for " << path << " : " << strerror(errno) << "\n";
        return;
    }

    struct dirent *entry;
    if (detailed) {
        cout << left << setw(12) << "PERMISSIONS" 
             << setw(8) << "OWNER" << setw(8) << "GROUP" 
             << setw(10) << "SIZE" << setw(20) << "MODIFIED" 
             << " NAME\n";
        cout << string(80, '-') << "\n";
    } else {
        cout << "\nContents of " << path << ":\n";
    }

    vector<string> names;
    while ((entry = readdir(dir)) != nullptr) {
        // skip . and ..
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        names.push_back(entry->d_name);
    }
    closedir(dir);

    // sort names for nicer output
    sort(names.begin(), names.end());
    for (auto &name : names) {
        if (detailed) showFileDetails(path, name);
        else cout << "  - " << name << "\n";
    }
}

// ---------- Create an empty file ----------
void createFile(const string &name) {
    FILE *f = fopen(name.c_str(), "wx"); // fail if exists
    if (!f) {
        // if file exists, try to open/truncate (or inform)
        if (errno == EEXIST) {
            cout << "File already exists: " << name << "\n";
        } else {
            cerr << "fopen failed: " << strerror(errno) << "\n";
        }
        return;
    }
    fclose(f);
    cout << "File created: " << name << "\n";
}

// ---------- Delete a file ----------
void deleteFile(const string &name) {
    if (remove(name.c_str()) == 0) {
        cout << "Deleted: " << name << "\n";
    } else {
        cerr << "remove failed: " << strerror(errno) << "\n";
    }
}

// ---------- Change working directory ----------
void changeDirectory(const string &path) {
    if (chdir(path.c_str()) == 0) {
        char buf[512];
        getcwd(buf, sizeof(buf));
        cout << "Changed directory to: " << buf << "\n";
    } else {
        cerr << "chdir failed: " << strerror(errno) << "\n";
    }
}

// ---------- Recursive search for a filename (prints matches) ----------
void searchFile(const string &dirname, const string &target) {
    DIR *dir = opendir(dirname.c_str());
    if (!dir) return; // silently ignore if cannot open

    struct dirent *entry;
    while ((entry = readdir(dir)) != nullptr) {
        string name = entry->d_name;
        if (name == "." || name == "..") continue;

        string full = dirname + "/" + name;

        if (entry->d_type == DT_DIR) {
            // Recurse into directory
            searchFile(full, target);
        } else {
            if (name == target) {
                cout << "Found: " << full << "\n";
            }
        }
    }
    closedir(dir);
}

// ---------- MAIN MENU ----------
int main() {
    cout << "=====================================\n";
    cout << "        🗂️ File Explorer Tool         \n";
    cout << "=====================================\n";

    while (true) {
        char cwd[512];
        getcwd(cwd, sizeof(cwd));
        cout << "\nCurrent Directory: " << cwd << "\n";
        cout << "1. List files (names only)\n";
        cout << "2. List files (detailed -> permissions, owner, size, mtime)\n";
        cout << "3. Create file\n";
        cout << "4. Delete file\n";
        cout << "5. Change directory\n";
        cout << "6. Search file (recursive)\n";
        cout << "7. Exit\n";
        cout << "Enter choice: ";

        int choice;
        if (!(cin >> choice)) {
            // if input fails, clear and continue
            cin.clear();
            cin.ignore(10000, '\n');
            cout << "Invalid input\n";
            continue;
        }

        string name, path;
        switch (choice) {
            case 1:
                listFiles(string(cwd), false);
                break;
            case 2:
                listFiles(string(cwd), true);
                break;
            case 3:
                cout << "Enter filename to create: ";
                cin >> name;
                createFile(name);
                break;
            case 4:
                cout << "Enter filename to delete: ";
                cin >> name;
                deleteFile(name);
                break;
            case 5:
                cout << "Enter directory to change to (absolute or relative): ";
                cin >> path;
                changeDirectory(path);
                break;
            case 6:
                cout << "Enter filename to search for (exact name): ";
                cin >> name;
                cout << "Searching (this may take time for large trees)...\n";
                searchFile(string(cwd), name);
                break;
            case 7:
                cout << "Goodbye!\n";
                return 0;
            default:
                cout << "Invalid choice\n";
        }
    }
    return 0;
}

