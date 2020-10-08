#include <iostream>
#include <utility>
#include <fstream>
#include <string>
#include <string.h>
#include <sys/stat.h>
#include <experimental/filesystem>
#include <vector>
#include <time.h>
#include <openssl/sha.h>

namespace fs = std::filesystem;

std::string root = "";

void init(std::string);
void addToCache(std::string, char);
bool ignoreFolder(std::string, std::vector<std::string>);
bool toBeIgnored(std::string, int);
void add(char *argv);
void addCommit(std::string, char, std::string);
void updateCommitLog(std::string, std::string);
void repeatCommit(std::string, char, std::string);
std::string getTime();
std::string getCommitHash();
void commit(std::string);
void getCommitLog();

void init(std::string path)
{

    struct stat buffer;

    if (stat((path + "/.imperium").c_str(), &buffer) == 0)
    {
        std::cout << "Repository has already been initialized!\n";
    }
    else
    {

        std::string ignorepath = path + "/.imperiumignore";
        std::ofstream ignore(ignorepath.c_str());

        ignore << ".gitignore\n.git\n.imperiumignore\n.imperium/\n.node_modules/\n";

        ignore.close();

        path += "/.imperium";
        int created = mkdir(path.c_str(), 0777);

        if (created == 0)
        {

            std::string commitlog = path + "/commit.log";
            std::ofstream commit(commitlog.c_str());
            commit.close();

            std::string addlog = path + "/add.log";
            std::ofstream add(addlog.c_str());
            add.close();

            std::string conflictlog = path + "/conflict.log";
            std::ofstream conflict(conflictlog.c_str());
            conflict.close();

            std::cout << "Initialized an empty repository!!\n";
        }
        else
        {
            std::cout << "ERROR\n";
        }
    }
}

void addToCache(std::string path, char type)
{
    struct stat buffer;
    if (stat((root + "/.imperium/.add").c_str(), &buffer) != 0)
    {
        mkdir((root + "/.imperium/.add").c_str(), 0777);
    }
    if (type == 'f')
    {
        std::string filename = path.substr(root.length());
        std::string filerel = root + "/.imperium/.add" + filename.substr(0, filename.find_last_of("/"));

        struct stat buffer2;
        if (stat(filerel.c_str(), &buffer2) != 0)
        {
            fs::create_directories(filerel);
        }
        fs::copy_file(path, root + "/.imperium/.add" + filename, fs::copy_options::overwrite_existing);
    }
    else if (type == 'd')
    {
        std::string filename = path.substr(root.length());
        std::string filerel = root + "/.imperium/.add" + filename;

        struct stat buffer3;
        if (stat(filerel.c_str(), &buffer3) != 0)
        {
            fs::create_directories(filerel);
        }
    }
}

bool ignoreFolder(std::string path, std::vector<std::string> dirname)
{
    for (auto dir : dirname)
    {
        dir.pop_back();

        if (path.find(dir) != std::string::npos)
        {
            return true;
        }
    }
    return false;
}

bool toBeIgnored(std::string path, int onlyImperiumIgnore = 0)
{
    std::ifstream addFile, ignoreFile;
    std::string file_or_dir;
    std::vector<std::string> filenames;
    std::vector<std::pair<std::string, char>> addedFileNames;
    std::vector<std::string> ignoreDir;

    ignoreFile.open(root + "/.imperiumignore");
    addFile.open(root + "/.imperium/add.log");

    if (ignoreFile.is_open())
    {

        while (!ignoreFile.eof())
        {
            std::getline(ignoreFile, file_or_dir);
            auto i = file_or_dir.end();

            i--;
            if (*i == '/')
            {
                ignoreDir.push_back(file_or_dir);
            }
            else
            {
                filenames.push_back(root + "/" + file_or_dir);
            }
        }
    }
    ignoreFile.close();

    if (onlyImperiumIgnore == 0)
    {
        if (addFile.is_open())
        {
            while (!addFile.eof())
            {
                std::getline(addFile, file_or_dir);
                if (file_or_dir.length() > 4)
                {
                    addedFileNames.push_back(std::make_pair(file_or_dir.substr(1, file_or_dir.length() - 4), file_or_dir.at(file_or_dir.length() - 1)));
                    // std::cout << "test: " << file_or_dir.substr(0, file_or_dir.length() - 2) << "\n";
                }
            }
        }
        addFile.close();

        for (auto fileEntry : addedFileNames)
        {
            if (path.compare(fileEntry.first) == 0)
            {
                addToCache(path, fileEntry.second);
                std::cout << "Updated: " << path << std::endl;
                return true;
            }
        }
    }
    if ((std::find(filenames.begin(), filenames.end(), path)) != filenames.end() || ignoreFolder(path, ignoreDir))
    {
        return true;
    }
    // for (auto filename : filenames)
    // {
    //     std::cout << "Compared .imperiumignore:\n"
    //               << filename << "\n"
    //               << path << "\n\n";
    // }
    return false;
}

void add(char *argv)
{
    struct stat buffer;
    if (stat((root + "/.imperium").c_str(), &buffer) == 0)
    {
        std::ofstream addFile;
        addFile.open(root + "/.imperium/add.log", std::ios_base::app);
        std::string path = root;
        if (strcmp(argv, ".") != 0)
        {
            path = path + "/" + argv;
        }
        struct stat buffer2;
        if (stat(path.c_str(), &buffer2) == 0)
        {
            if (buffer2.st_mode & S_IFDIR)
            {
                if (!toBeIgnored(path))
                {
                    addFile << "\"" << path << "\""
                            << "-d\n";
                    addToCache(path, 'd');
                    std::cout << "Added directory: "
                              << "\"" << path << "\""
                              << "\n";
                }
                for (auto &p : fs::recursive_directory_iterator(path))
                {
                    if (toBeIgnored(p.path()))
                        continue;
                    struct stat buffer3;
                    if (stat(p.path().c_str(), &buffer3) == 0)
                    {

                        if (buffer3.st_mode & S_IFDIR)
                        {
                            addToCache(p.path(), 'd');
                            addFile << p.path() << "-d\n";
                            std::cout << "Added directory: " << p.path() << "\n";
                        }
                        else if (buffer3.st_mode & S_IFREG)
                        {
                            addToCache(p.path(), 'f');
                            addFile << p.path() << "-f\n";
                            std::cout << "Added file: " << p.path() << "\n";
                        }
                        else
                        {
                            continue;
                        }
                    }
                }
            }
            else if (buffer2.st_mode & S_IFREG)
            {
                if (!toBeIgnored(path))
                {
                    addToCache(path, 'f');
                    addFile << "\"" << path << "\""
                            << "-f\n";
                    std::cout << "Added file: "
                              << "\"" << path << "\""
                              << "\n";
                }
            }
            else
            {
                std::cout << "Invalid path!\n";
            }
        }
    }
    else
    {
        std::cout << "Repository has not been initialised yet.\n";
    }
}

void addCommit(std::string abspath, char type, std::string commitHash)
{
    struct stat s;
    if (stat((root + "/.imperium/.commit").c_str(), &s) != 0)
    {
        mkdir((root + "/.imperium/.commit").c_str(), 0777);
    }
    if (stat((root + "/.imperium/.commit/" + commitHash).c_str(), &s) != 0)
    {
        mkdir((root + "/.imperium/.commit/" + commitHash).c_str(), 0777);
    }

    std::string relpath = abspath.substr(root.length() + 15);
    std::string filepath = root + "/.imperium/.commit/" + commitHash + relpath.substr(0, relpath.find_last_of('/'));

    fs::create_directories(filepath);

    if (type == 'f')
    {
        fs::copy_file(abspath, root + "/.imperium/.commit/" + commitHash + relpath, fs::copy_options::overwrite_existing);
    }
}

void updateCommitLog(std::string commitHash, std::string message)
{
    std::ofstream writeHeadLog;

    writeHeadLog.open(root + "/.imperium/head.log");
    writeHeadLog << commitHash << " -- " << message << std::endl;
    writeHeadLog.close();

    std::ofstream writeCommitLog;
    std::ifstream readCommitLog;

    readCommitLog.open(root + "/.imperium/commit.log");
    writeCommitLog.open(root + "/.imperium/new_commit.log");

    writeCommitLog << commitHash << " -- " << message << " -->HEAD\n";

    std::string line;
    while (std::getline(readCommitLog, line))
    {
        if (line.find(" -->HEAD") != std::string::npos)
        {
            writeCommitLog << line.substr(0, line.length() - 8) << "\n";
        }
        else
        {
            writeCommitLog << line << "\n";
        }
    }

    remove((root + "/.imperium/commit.log").c_str());
    rename((root + "/.imperium/new_commit.log").c_str(), (root + "/.imperium/commit.log").c_str());

    readCommitLog.close();
    writeCommitLog.close();
}

void repeatCommit(std::string abspath, char type, std::string commitHash)
{
    mkdir((root + "/.imperium/.commit/" + commitHash).c_str(), 0777);

    std::string relpath = abspath.substr(root.length() + 19 + commitHash.length());
    std::string filepath = root + "/.imperium/.commit/" + commitHash + relpath.substr(0, relpath.find_last_of('/'));

    fs::create_directories(filepath);

    if (type == 'f')
    {
        fs::copy_file(abspath, root + "/.imperium/.commit/" + commitHash + relpath, fs::copy_options::overwrite_existing);
    }
}

std::string getTime()
{
    auto end = std::chrono::system_clock::now();
    std::time_t end_time = std::chrono::system_clock::to_time_t(end);
    std::string time = std::ctime(&end_time);

    return time;
}

std::string getCommitHash()
{
    std::string commitfilename = getTime();
    std::string commitHash = "";

    char buf[3];
    int length = commitfilename.length();
    unsigned char hash[20];
    unsigned char *val = new unsigned char[length + 1];
    strcpy((char *)val, commitfilename.c_str());

    SHA1(val, length, hash);
    for (int i = 0; i < 20; i++)
    {
        sprintf(buf, "%02x", hash[i]);
        commitHash += buf;
    }
    return commitHash;
}

void commit(std::string message)
{
    struct stat buffer;
    if (stat((root + "/.imperium").c_str(), &buffer) != 0)
    {
        std::cout << "Repository has not been initialised!\n";
        return;
    }

    std::string commitHash = getCommitHash();

    //Copy previous commits
    if (stat((root + "/.imperium/head.log").c_str(), &buffer) == 0)
    {
        std::string headHash;
        std::ifstream readCommitLog;

        readCommitLog.open(root + "/.imperium/head.log");

        std::getline(readCommitLog, headHash);
        headHash = headHash.substr(0, headHash.find(" -- "));
        for (auto &p : fs::recursive_directory_iterator(root + "/.imperium/.commit/" + headHash))
        {

            if (stat(p.path().c_str(), &buffer) == 0)
            {
                if (buffer.st_mode & S_IFREG)
                {
                    repeatCommit(p.path(), 'f', commitHash);
                }
                else if (buffer.st_mode & S_IFDIR)
                {
                    repeatCommit(p.path(), 'd', commitHash);
                }
                else
                {
                    continue;
                }
            }
        }
    }

    for (auto &p : fs::recursive_directory_iterator(root + "/.imperium/.add"))
    {
        struct stat s;
        if (stat(p.path().c_str(), &s) == 0)
        {
            if (s.st_mode & S_IFREG)
            {
                addCommit(p.path(), 'f', commitHash);
            }
            else if (s.st_mode & S_IFDIR)
            {
                addCommit(p.path(), 'd', commitHash);
            }
        }
    }

    fs::remove_all((root + "/.imperium/.add").c_str());
    remove((root + "/.imperium/add.log").c_str());
    updateCommitLog(commitHash, message);
}

void getCommitLog()
{
    std::string commitlogpath = root + "/.imperium/commit.log";
    std::string commitline;
    std::ifstream commitlog;

    commitlog.open(commitlogpath);
    while (std::getline(commitlog, commitline))
    {
        std::cout << commitline << std::endl;
    }
}

int main(int argc, char *argv[])
{

    const std::string dir = getenv("dir");
    root = dir;

    if (strcmp(argv[1], "init") == 0)
    {
        init(root);
    }
    else if (strcmp(argv[1], "add") == 0)
    {
        if (strcmp(argv[2], ".") != 0)
        {
            for (int i = 2; i < argc; i++)
            {
                add(argv[i]);
            }
        }
        else
        {
            add(argv[2]);
        }
    }
    else if (strcmp(argv[1], "commit") == 0)
    {
        commit(argv[2]);
    }
    else if (strcmp(argv[1], "log") == 0)
    {
        getCommitLog();
    }

    return 0;
}