#include <sys/types.h>
#include <dirent.h>
#include <iostream>
#include <sys/stat.h>
#include <unistd.h>
#include <cstring>
#include <fcntl.h>

using namespace std;

int skip(char *);
void readDirRecurse(const string &);

int main(int argc, char **argv) {
    umask(0000);
    char *dirName = argv[1];

    string dirNameString = string(dirName).substr(0, dirNameString.size() - 1) + string(".arch");
    open(dirNameString.c_str(), O_WRONLY | O_TRUNC, 0700);
    readDirRecurse(string(dirName));
    return 0;
}

int skip(char *fileName) {
    return (strncmp(fileName, "..", 255) == 0)
           || (strncmp(fileName, ".", 255) == 0);
}

void readDirRecurse(const string &path) {
    DIR *dir = opendir(path.c_str());
    struct dirent *dirent;

    while ((dirent = readdir(dir)) != nullptr) {
        if (skip(dirent->d_name))
            continue;
        struct stat s{};
        string pathToFile = path + (dirent->d_name);
        stat(pathToFile.c_str(), &s);
        pathToFile += (S_ISDIR(s.st_mode) ? "/" : "");
        size_t foundFirstSlash = pathToFile.find_first_of("/\\");
        string relative = "./" + pathToFile.substr(foundFirstSlash + 1);

        int tarFd = open("test.arch", O_WRONLY | O_CREAT | O_APPEND, 0700); // open arch file
        char fileBuffer[s.st_size]; // buffer with capacity == size of file
        unsigned int pathSize[] = {(unsigned int) relative.size()}; // holds path size
        unsigned int dataSize[1]; // holds data size
        write(tarFd, &pathSize, 4); // write size  of pathname

        if (!S_ISDIR(s.st_mode)) {
            dataSize[0] = (unsigned int) s.st_size; // data size

            int fd = open(pathToFile.c_str(), O_RDONLY); // open file
            read(fd, &fileBuffer, (size_t) (s.st_size)); // read contents of file into buffer
            write(tarFd, &dataSize, 4); // write size of data
        }

        write(tarFd, relative.c_str(), relative.size()); // write pathname

        if (!S_ISDIR(s.st_mode))
            write(tarFd, &fileBuffer, (size_t) (s.st_size)); // write data

        if (S_ISDIR(s.st_mode))
            readDirRecurse(path + dirent->d_name + "/");
    }

    closedir(dir);
}

//pathname size: a 4 byte unsigned integer
//data size: a 4 byte unsigned integer
//pathname: ASCII string containing the relative path from the source directory to the file; it is not null terminated
//data: the contents of the file

