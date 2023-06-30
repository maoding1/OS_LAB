//
// Created by mao on 23-4-14.
//
#include <iostream>
#include <fstream>
#include <string.h>
#include <vector>
#include <sstream>
#include <regex>

#define BPB_LENGTH 36
#define ROOT_ENTRY_LENGTH 32
using namespace std;

typedef char u8;
typedef unsigned short u16; //2字节
typedef unsigned int u32;   //4字节

#pragma pack(1)
//BIOS Parameter Block 的抽象
struct BPB {
    u8 BPB_UselessInfo[11];
    u16 BPB_BytesPerSec;
    u8 BPB_SecPerClus;
    u16 BPB_ReservedSecCnt;
    u8 BPB_FATCnt;
    u16 BPB_RootDirEntryCnt;
    u16 BPB_TotalSecCnt1;
    u8 BPB_MediaDesType;
    u16 BPB_SecPerFAT;
    u16 BPB_SecPerTrack;
    u16 BPB_HeadsCnt;
    u32 BPB_HiddenSecCnt;
    u32 BPB_TotalSecCnt2;
}bpb;
// 根目录项的抽象
struct RootEntry {
    char name[11];
    u8 attr;
    char uselessInfo[14];
    u16 startClus; //文件开始簇号
    u32 fileSize; //文件大小
};
#pragma pack()
//文件的抽象，包括普通文件和目录
struct file {
    RootEntry fileEntry;
    char roadName[50] = {0};
    char fileName[50] = {0};
    // 在fileName后追加字符串
    void fappend(const char *ch) {strcat(fileName, ch);}
    // 在roadName后追加字符串
    void rappend(const char *ch) {strcat(roadName, ch);}
};
// 包含子目录和子文件个数的文件
struct lsans {
    bool flag;
    char fileName[12];
    int cntr, cntf, fsize;
    void append(char *ch) {strcat(fileName, ch);}
    lsans() {
        cntr = cntf = 0;
        memset(fileName, 0, sizeof(fileName));
    }
};
int BytesPerSec, SecPerClus, ReservedSecCnt, FATCnt, RootDirEntryCnt, SecPerFAT, BytesPerClus;
int fatByteBase;        //引导扇区的起始字节偏移
int fileRootByteBase;   //根目录区的起始字节偏移
int dataByteBase;       //数据区的起始字节偏移
char tmp_filename[11];  //临时的文件名
int rootFileCnt = 0, rootDirCnt = 0; //记录根目录下的文件数和目录数
char qfroot[30];        //保存命令中的文件路径
char text[10000];       //保存文件内容
vector<file> rootFiles; //根目录下的文件集合
vector<file> fileTree;  //目录树
vector<lsans> vcans;    //暂存某个目录下的文件集合
void getFileName(const char* filename);
void getDirName(const char* roadname);
void dfs(FILE *fat12, file cur, char *roadname);
int getNextFAT(FILE *fat12, int num);
void parsecmd(FILE *fat12, char *cmd);
void ls(FILE *fat12);
void ls_l(FILE *fat12);
int getqname(const char*q, int st);
bool validPathLetter(char ch);
bool is_standard_file(RootEntry re);
bool validFileName(char *fname_tmp, int pos);
void lslr(FILE *fat12);
void lsr(FILE *fat12);
void solveCat(FILE *fat12);
void getText(FILE *fat12, RootEntry re);
void getTextEntry(FILE *fat12, RootEntry re, string tname);
vector<string> split(char* str, char delimiter);
string join(vector<string> tokens, char delimiter);
bool isTXT(const std::string& filename);
pair<int, int> solveRootFile(file cur, FILE *fat12);
pair<int, int> getCnt(RootEntry re, FILE *fat12);
extern "C" void my_print(char *, unsigned int, int);
//void my_print(char *str, int length, int flag) {
//    for (int i=0; i < length; i++)
//        cout << str[i];
//    if (flag == 1)
//        cout << "(red)";
//}
//检查是否为文件
bool is_standard_file(RootEntry re) {
    return re.attr == 0x01 || re.attr == 0x10 || re.attr == 0x20;
}

bool validFileName(char *fname_tmp, int pos) {
    for (int j = pos; j < pos + 11; j++) {
        if (!(((fname_tmp[j] >= 48) && (fname_tmp[j] <= 57)) ||
              ((fname_tmp[j] >= 65) && (fname_tmp[j] <= 90)) ||
              ((fname_tmp[j] >= 97) && (fname_tmp[j] <= 122)) ||
              (fname_tmp[j] == ' ')))
            return false;
    }
    return true;
}
//返回用delimiter分隔的字符串str的vector容器
vector<string> split(char* str, char delimiter) {
    vector<string> tokens;
    istringstream iss(str);
    string token;
    while (getline(iss, token, delimiter)) {
        if (token != "")
            tokens.push_back(token);
    }
    return tokens;
}

string join(vector<string> tokens, char delimiter) {
    string ret = "";
    for (const string& token : tokens) {
        ret = ret + delimiter + token;
    }
    return ret;
}

bool isTXT(const string& filename) {
    std::regex pattern(R"((.*).TXT)", std::regex::icase);
    return std::regex_match(filename, pattern); // 返回是否匹配成功
}
int get_BPB_info() {
    BytesPerSec = bpb.BPB_BytesPerSec;
    SecPerClus = bpb.BPB_SecPerClus;
    ReservedSecCnt = bpb.BPB_ReservedSecCnt;
    FATCnt = bpb.BPB_FATCnt;
    RootDirEntryCnt = bpb.BPB_RootDirEntryCnt;
    SecPerFAT = bpb.BPB_SecPerFAT;
    BytesPerClus = SecPerClus * BytesPerSec;

    fatByteBase = ReservedSecCnt * BytesPerSec;
    fileRootByteBase = (ReservedSecCnt + FATCnt * SecPerFAT) * BytesPerSec;
    dataByteBase = (ReservedSecCnt + FATCnt * SecPerFAT + (RootDirEntryCnt * 32 + BytesPerSec - 1) / BytesPerSec) * BytesPerClus;
}
// 把根目录下的文件和目录存到rootFiles中
void getRootFiles(FILE *fat12) {
    RootEntry rootEntry;
    for (int i = 0, curBase = fileRootByteBase; i < RootDirEntryCnt; i++, curBase += ROOT_ENTRY_LENGTH) {
        fseek(fat12, curBase, SEEK_SET);
        fread(&rootEntry, 1, ROOT_ENTRY_LENGTH, fat12);
        if (!is_standard_file(rootEntry) || !validFileName(rootEntry.name, 0)) //检查是否为空或已删除
            continue;
        file f;
        if (rootEntry.attr != 0x10) { //rootEntry描述的是文件
            getFileName(rootEntry.name);
            rootFileCnt ++;
        } else {                      //rootEntry描述的是目录
            getDirName(rootEntry.name);
            f.rappend("/");
            rootDirCnt++;
        }
        f.fappend(tmp_filename);
        f.fileEntry = rootEntry;
        rootFiles.push_back(f);
    }
}
// 如果ch是小写字母 转为大写
char upper(char ch) {
    if ( ch >= 'a' && ch <= 'z')
        ch -= 32;
    return ch;
}
//将filename表示的文件名存储到tmp_filename中
void getFileName(const char* filename) {
    int curlen = 0;
    for (int i = 0; i < 11; i++) {
        if (filename[i] != ' ')
            tmp_filename[curlen++] = upper(filename[i]);
        else {
            tmp_filename[curlen++] = '.';
            while (filename[i] == ' ' &&  i < 11)
                i++;
            i--;
        }
    }
    tmp_filename[curlen] = '\0';
}
//将roadname表示的路径存储到tmp_filename中
void getDirName(const char* roadname) {
    int curlen = 0;
    for (int i = 0; i < 11; i++) {
        if (roadname[i] == ' ')
            break;
        tmp_filename[curlen++] = upper(roadname[i]);
    }
    tmp_filename[curlen] = '\0';
}
//深度优先遍历roadname表示的目录下的目录， 并加入到fileTree中
void dfs(FILE *fat12, file cur, char *roadname) {
    int curClus = cur.fileEntry.startClus;
    int startByte;
    while (curClus < 0xFF8) {
        if (curClus == 0xFF7) {
            char message[40] = "Couldn't read bad cluster!\n";
            my_print(message, strlen(message), 1);
            break;
        }
        startByte = dataByteBase + (curClus - 2) * BytesPerClus;
        for (int i = 0; i < BytesPerClus; i += ROOT_ENTRY_LENGTH) {
            RootEntry rootEntry;
            fseek(fat12, startByte + i, SEEK_SET);
            fread(&rootEntry, 1, ROOT_ENTRY_LENGTH, fat12);
            if (!is_standard_file(rootEntry) || rootEntry.attr != 0x10 || !validFileName(rootEntry.name, 0))//非合法表项或是文件或跳过
                continue;
            getDirName(rootEntry.name);
            file f;
            f.fappend(tmp_filename);
            f.rappend(roadname);
            f.rappend("/");
            f.fileEntry = rootEntry;
            char tmp[50];
            memset(tmp, 0, sizeof(tmp));
            strcat(tmp, f.roadName);
            strcat(tmp, f.fileName);
            fileTree.push_back(f);
            dfs(fat12, f, tmp);
        }
        curClus = getNextFAT(fat12, curClus);
    }
}
int getNextFAT(FILE *fat12, int num) {
    u16 bytes;
    fseek(fat12, fatByteBase + num * 3 / 2, SEEK_SET);
    fread(&bytes, 1, 2, fat12);
    //偶数去掉高4位 奇数去掉低4位
    return (num & 1) ? (bytes >> 4) : (bytes & ((1 << 12) - 1));
}

void parsecmd(FILE *fat12, char *cmd) {
    int len = strlen(cmd);
    bool isls = true, lsf1 = false, lsf2 = false;
    int comeFirst; //检测先出现的是-还是/
    int i = 0;
    //检测非法输入
    while (i < len && cmd[i] == ' ')
        i++;
    if (cmd[i] != 'l' && cmd[i] != 'c') {
        char wrongMessage1[50] = "Please enter the correct command.\n";
        my_print(wrongMessage1, strlen(wrongMessage1), 1);
        return;
    }
    // 如果是ls命令
    if (cmd[i] == 'l' && cmd[i+1] == 's') {
        i += 2;
        for (int j = i; j < len; j++) {
            if (cmd[j] != ' ')
                isls = false;
        }
        // 如果只有ls
        if (isls) {
            ls(fat12);
            return;
        }
        //如果ls后是空格，去掉ls后的空格
        if (cmd[i] == ' ') {
            while (i < len && cmd[i] == ' ')
                i++;
        } else { //ls后面不是空格
            char wrongMessage2[50] = "Not a legal command!\n";
            my_print(wrongMessage2, strlen(wrongMessage2), 1);
            return;
        }
        //如果是ls -l
        if (cmd[i] == '-' && cmd[i+1] == 'l') {
            i += 2;
            //去除 -l 后多余的l
            while (i < len && cmd[i] == 'l')
                i++;
            //如果 是 ls -l 没有文件名
            if (i == len) {
                ls_l(fat12);
                return;
            }
            //去除 -l 后的空格
            if (cmd[i] == ' ') {
                while (i < len && cmd[i] == ' ')
                    i++;
                if (i == len) { //是 ls -l 后面没有文件名
                    ls_l(fat12);
                    return;
                } else if (cmd[i] == '/'){ //ls -l 后面加了路径
                    getqname(cmd, i);
                    lslr(fat12);
                }
            } else { // -l后有多余的字符
                char wrongMessage2[50] = "Not a legal command!\n";
                my_print(wrongMessage2, strlen(wrongMessage2), 1);
                return;
            }

        } else if (cmd[i] == '/') { //ls后面接路径
            int res = getqname(cmd, i);
            if (res == 1)
                lslr(fat12);
            else if(res == 2)
                lsr(fat12);
            else
                return;
        } else {
            char wrongMessage2[50] = "Not a legal command!\n";
            my_print(wrongMessage2, strlen(wrongMessage2), 1);
            return;
        }
    }
    if (cmd[i] == 'c' && cmd[i+1] == 'a' && cmd[i+2] == 't') {
        i += 3;
        while (i < len && cmd[i] == ' ')
            i++;
        if(getqname(cmd, i) != 0)
            solveCat(fat12);
    }
}
void ls(FILE *fat12) {
    pair<int, int> ptmp;
    int mode;
    my_print("/:\n", 3, 0);
    for (auto _file: rootFiles) {
        mode = (_file.fileEntry.attr == 0x10 ? 1 : 0);
        my_print(_file.fileName, strlen(_file.fileName), mode);
        my_print("  ", 2, 0);
    }
    my_print("\n", 1, 0);

    for (auto _file: fileTree) {
        solveRootFile(_file, fat12);
        my_print(_file.roadName, strlen(_file.roadName), 0);
        my_print(_file.fileName, strlen(_file.fileName), 0);
        my_print("/:", 2, 0);
        my_print("\n", 1, 0);
        my_print(".  ..  ", 7, 1);
        for (auto childFile : vcans) {
            my_print(childFile.fileName, strlen(childFile.fileName), childFile.flag);
            my_print("  ",2,0);
        }
        my_print("\n", 1, 0);
    }
}
// 将目录cur下的文件保存到vcans中 返回<直接子目录数，直接子文件数>
pair<int, int> solveRootFile(file cur, FILE *fat12) {
    vcans.clear();
    int ans1 = 0, ans2 = 0;
    int curClus = cur.fileEntry.startClus, startByte = 0;
    while (curClus < 0xFF8) {
        if (curClus == 0xFF7) {
            char message[40] = "Couldn't read bad cluster!\n";
            my_print(message, strlen(message), 1);
            break;
        }
        startByte = dataByteBase + (curClus - 2) * BytesPerClus;
        for (int i = 0; i < BytesPerClus; i += ROOT_ENTRY_LENGTH) {
            RootEntry rootEntry;
            fseek(fat12, startByte + i, SEEK_SET);
            fread(&rootEntry, 1, ROOT_ENTRY_LENGTH, fat12);
            if (!is_standard_file(rootEntry) || !validFileName(rootEntry.name, 0)) //非合法文件
                continue;
            if (rootEntry.attr != 0x10) {   //是文件
                lsans file;
                file.flag = 0;
                ans2++;
                getFileName(rootEntry.name);
                file.append(tmp_filename);
                file.fsize = rootEntry.fileSize;
                vcans.push_back(file);
            } else {
                //是目录
                lsans file;
                file.flag = 1;
                ans1++;
                getDirName(rootEntry.name);
                file.append(tmp_filename);
                pair<int, int> ans = getCnt(rootEntry, fat12);
                file.cntr = ans.first;
                file.cntf = ans.second;
                vcans.push_back(file);
            }
        }
        curClus = getNextFAT(fat12, curClus);
    }
    return {ans1, ans2};
}
//获取re表示的目录下的直接子目录数和直接子文件数
pair<int, int> getCnt(RootEntry re, FILE *fat12) {
    pair<int, int> ans;
    ans.first = 0, ans.second = 0;
    int curClus = re.startClus;
    int startByte = 0;
    while (curClus < 0xFF8) {
        if (curClus == 0xFF7) {
            char message[40] = "Couldn't read bad cluster!\n";
            my_print(message, strlen(message), 1);
            break;
        }
        startByte = dataByteBase + (curClus - 2) * BytesPerClus;
        for (int i = 0; i < BytesPerClus; i += ROOT_ENTRY_LENGTH) {
            RootEntry rootEntry;
            fseek(fat12, startByte + i, SEEK_SET);
            fread(&rootEntry, 1, ROOT_ENTRY_LENGTH, fat12);
            if (!is_standard_file(rootEntry) || !validFileName(rootEntry.name, 0)) // 非合法文件
                continue;
            if (rootEntry.attr != 0x10)
                ans.second++;
            else
                ans.first++;
        }
        curClus = getNextFAT(fat12, curClus);
    }
    return ans;
}

void ls_l(FILE *fat12) {
    my_print("/ ", 2, 0);
    char fileCnts[100];
    sprintf(fileCnts, "%d", rootDirCnt);
    my_print(fileCnts, strlen(fileCnts), 0);
    my_print(" ", 1, 0);
    sprintf(fileCnts, "%d", rootFileCnt);
    my_print(fileCnts, strlen(fileCnts), 0);
    my_print(":\n", 2, 0);
    for (auto _file : rootFiles) {
        if (_file.fileEntry.attr != 0x10) {
            //是文件
            my_print(_file.fileName, strlen(_file.fileName), 0);
            my_print(" ", 1, 0);
            sprintf(fileCnts, "%d", _file.fileEntry.fileSize);
            my_print(fileCnts, strlen(fileCnts), 0);
            my_print("\n", 1, 0);
        } else {
            pair<int ,int> ans = getCnt(_file.fileEntry, fat12);
            my_print(_file.fileName, strlen(_file.fileName), 1);
            my_print(" ", 1, 0);
            sprintf(fileCnts, "%d", ans.first);
            my_print(fileCnts, strlen(fileCnts), 0);
            my_print(" ", 1, 0);
            sprintf(fileCnts, "%d", ans.second);
            my_print(fileCnts, strlen(fileCnts), 0);
            my_print("\n", 1, 0);
        }
    }
    my_print("\n", 1, 0);
    for (auto _file: fileTree) {
        pair<int, int> rans = solveRootFile(_file, fat12);
        my_print(_file.roadName, strlen(_file.roadName), 0);
        my_print(_file.fileName, strlen(_file.fileName), 0);
        my_print("/ ", 2, 0);
        sprintf(fileCnts, "%d", rans.first);
        my_print(fileCnts, strlen(fileCnts), 0);
        my_print(" ", 1, 0);
        sprintf(fileCnts, "%d", rans.second);
        my_print(fileCnts, strlen(fileCnts), 0);
        my_print(":\n", 2, 0);
        my_print(".\n..\n", 5, 1);
        for (auto child : vcans) {
            if (child.flag == 1) {
                my_print(child.fileName, strlen(child.fileName), 1);
                my_print(" ", 1, 0);
                sprintf(fileCnts, "%d", child.cntr);
                my_print(fileCnts, strlen(fileCnts), 0);
                my_print(" ", 1, 0);
                sprintf(fileCnts, "%d", child.cntf);
                my_print(fileCnts, strlen(fileCnts), 0);
                my_print("\n", 1, 0);
            } else {
                my_print(child.fileName, strlen(child.fileName), 0);
                my_print(" ", 1, 0);
                sprintf(fileCnts, "%d", child.fsize);
                my_print(fileCnts, strlen(fileCnts), 0);
                my_print("\n", 1, 0);
            }
        }
        my_print("\n", 1, 0);
    }
}
//返回0命令出错, 返回1代表命令中有-l, 返回2代表命令中没有-l 并把命令中的路径部分存储到qfroot中
int getqname(const char*q, int pos) {
    int len = strlen(q);
    int dot = 0;
    int j = pos;
    int qlen = 0;
    bool ls = true;
    char wrongMessage[50] = "Not a legal command!\n";
    for (j = pos; j < len; j++) {
        if (q[j] == ' ')
            break;
        else if (!validPathLetter(q[j])) {
            char wrongmessage2[50] = "Please enter uppercase of path!\n";
            my_print(wrongmessage2, strlen(wrongmessage2), 1);
            return 0;
        }
        qfroot[qlen++] = q[j];
    }
    qfroot[qlen] = '\0';
    //检查路径是否合法
    vector<string> dirs = split(qfroot, '/');
    vector<string> curRoad;
    for (string dir : dirs) {
        if (dir == ".")
            continue;
        else if (dir == "..") {
            if(!curRoad.empty())
                curRoad.pop_back();
            continue;
        }
        curRoad.push_back(dir);
        //检查当前路径是否存在
        if (isTXT(dir))
            continue;
        string tmpRoad = join(curRoad, '/');
        if (tmpRoad == "") //是根目录
            continue;
        bool exist = false;
        for (file _file : fileTree) {
            string fileRoad = string(_file.roadName) + string(_file.fileName);
            if (fileRoad == tmpRoad) {
                exist = true;
                break;
            }
        }
        if (!exist) {
            char wrongmessage3[50] = "wrong file path!Dir not exist!\n";
            my_print(wrongmessage3, strlen(wrongmessage3), 1);
            return 0;
        }
    }
    string real_road = join(curRoad, '/');
    sprintf(qfroot, "%s", real_road.c_str());
    //去除路径后的空格
    while (j < len && q[j] == ' ')
        j++;
    if (j == len)
        return 2;

    if (j < len && q[j] == '-') {
        if (j < len-1 && q[j+1] == 'l') {   //是-l命令
            j++;
            ls = false;
            while (j < len && q[j] == 'l')  //去除多余的l命令
                j++;
            while (j < len && q[j] == ' ')  //去除命令后的空格
                j++;
            if (j == len)
                return 1;
            else {                          //有多余的字符
                my_print(wrongMessage, strlen(wrongMessage), 1);
                return 0;
            }
        } else {
            my_print(wrongMessage, strlen(wrongMessage), 1);
            return 0;
        }
    } else {
        my_print(wrongMessage, strlen(wrongMessage), 1);
        return 0;
    }
}
//检查ch是不是合法的路径字符
bool validPathLetter(char ch) {
    if (ch == '/' || (ch <= 'Z' && ch >= 'A') || (ch <= '9' && ch >= '0') || ch == '.')
        return true;
    return false;
}
//TODO: 算法可能有错误 主要是没有递归输出所有
void lslr(FILE *fat12) {
    //判断路径是不是根目录
    if (strcmp(qfroot, "") == 0) {
        ls_l(fat12);
        return;
    }
    bool isFind = false;
    for (auto _file : fileTree) {
        char tmp[50];
        memset(tmp, 0, sizeof(tmp));
        strcat(tmp, _file.roadName);
        strcat(tmp, _file.fileName);
        char fileCnts[100];
        if (strncmp(tmp, qfroot, strlen(qfroot)) == 0) {
            isFind = true;
            pair<int, int> ans = solveRootFile(_file, fat12);
            my_print(tmp, strlen(tmp), 0);
            my_print("/ ", 2, 0);
            sprintf(fileCnts, "%d", ans.first);
            my_print(fileCnts, strlen(fileCnts), 0);
            my_print(" ",1 ,0);
            sprintf(fileCnts, "%d", ans.second);
            my_print(fileCnts, strlen(fileCnts), 0);
            my_print(":\n",2 ,0);
            my_print(".\n..\n", 5, 1);
            for (auto child : vcans) {
                if (child.flag == 1) {
                    my_print(child.fileName, strlen(child.fileName), 1);
                    my_print(" ", 1, 0);
                    sprintf(fileCnts, "%d", child.cntr);
                    my_print(fileCnts, strlen(fileCnts), 0);
                    my_print(" ", 1, 0);
                    sprintf(fileCnts, "%d", child.cntf);
                    my_print(fileCnts, strlen(fileCnts), 0);
                    my_print("\n", 1, 0);
                } else {
                    my_print(child.fileName, strlen(child.fileName), 0);
                    my_print(" ", 1, 0);
                    sprintf(fileCnts, "%d", child.fsize);
                    my_print(fileCnts, strlen(fileCnts), 0);
                    my_print("\n", 1, 0);
                }
            }
            my_print("\n", 1, 0);
        }
    }
    if (!isFind) {
        char wrongmessage[50] = "no such file!\n";
        my_print(wrongmessage, strlen(wrongmessage), 0);
    }

}

void lsr(FILE *fat12) {
    //判断路径是不是根目录
    if (strcmp(qfroot, "") == 0) {
        ls(fat12);
        return;
    }
    bool isFind = false;
    for (auto _file: fileTree) {
        char tmp[50];
        memset(tmp, 0, sizeof(tmp));
        strcat(tmp, _file.roadName);
        strcat(tmp, _file.fileName);
        if (strncmp(tmp, qfroot, strlen(qfroot)) == 0) {
            isFind = true;
            solveRootFile(_file, fat12);
            my_print(tmp, strlen(tmp), 0);
            my_print("/:\n", 3, 0);
            my_print(". .. ", 5, 1);
            for (auto child: vcans) {
                my_print(child.fileName, strlen(child.fileName), child.flag);
                my_print(" ", 1, 0);
            }
            my_print("\n", 1, 0);
        }
    }
    if (!isFind) {
        char wrongmessage[50] = "no such file!\n";
        my_print(wrongmessage, strlen(wrongmessage), 0);
    }
}
void solveCat(FILE *fat12) {
    string str = qfroot, sroot = "", stext = "";
    bool flag = 0;
    int pos = str.rfind('/');
    if (pos <= 0) {
        stext = pos < 0 ? str : str.substr(pos+1);
        for (auto _file : rootFiles) {
            if (strcmp(_file.fileName, stext.c_str()) == 0) {
                getText(fat12, _file.fileEntry);
                flag = 1;
                break;
            }
        }
    }
    sroot = str.substr(0, pos);
    stext = str.substr(pos+1);
    for (auto _file : fileTree) {
        string x = _file.roadName;
        x += _file.fileName;
        if (x == sroot) {
            getTextEntry(fat12, _file.fileEntry, stext);
            flag = 1;
            break;
        }
    }
    if (flag == 0) {
        char wrongmessage[30] = "no such file!\n";
        my_print(wrongmessage, strlen(wrongmessage), 1);
    }
}

void getText(FILE *fat12, RootEntry re) {
    int curClus = re.startClus;
    int startByte;
    while (curClus < 0xFF8) {
        if (curClus == 0xFF7) {
            char message[40] = "Couldn't read bad cluster!\n";
            my_print(message, strlen(message), 1);
            break;
        }
        startByte = dataByteBase + (curClus - 2) * BytesPerClus;
        fseek(fat12, startByte, SEEK_SET);
        fread(text, 1, BytesPerClus, fat12);
        cout << text;
        curClus = getNextFAT(fat12, curClus);
    }
}

void getTextEntry(FILE *fat12, RootEntry re, string tname) {
    int curClus = re.startClus;
    int startByte;
    while (curClus < 0xFF8) {
        if (curClus == 0xFF7) {
            char message[40] = "Couldn't read bad cluster!\n";
            my_print(message, strlen(message), 1);
            break;
        }
        startByte = dataByteBase + (curClus - 2) * BytesPerClus;
        for (int i = 0; i < BytesPerClus; i += ROOT_ENTRY_LENGTH) {
            RootEntry rootEntry;
            fseek(fat12, startByte + i, SEEK_SET);
            fread(&rootEntry, 1, 32, fat12);
            if (rootEntry.name[0] == '\0' || rootEntry.name[0] == 0xE5 || rootEntry.attr == 0x10 || !validFileName(rootEntry.name, 0))
                continue;
            getFileName(rootEntry.name);
            string tmp = tmp_filename;
            if (tmp == tname) {
                getText(fat12, rootEntry);
                break;
            }
        }
        curClus = getNextFAT(fat12, curClus);
    }
}
int main() {
    FILE *fat12;
    //最好用绝对路径 编译器默认的工作目录未知
    fat12 = fopen("/home/mao/Desktop/os/lab2/b.img", "rb");
    fread(&bpb, 1, BPB_LENGTH, fat12);
    get_BPB_info();
    getRootFiles(fat12);
    for (auto _file : rootFiles) {
        if (_file.fileEntry.attr != 0x10) { //是普通文件
            continue;
        }
        char tmp[50];
        memset(tmp, 0, sizeof(tmp));
        strcat(tmp, _file.roadName);
        strcat(tmp, _file.fileName);
        fileTree.push_back(_file);
        dfs(fat12, _file, tmp);
    }
    char cmd[50];
    my_print(">", 1, 0);
    while (cin.getline(cmd, 50)) {
        if (!strcmp(cmd, "exit") || !strcmp(cmd, "EXIT"))
            break;
        parsecmd(fat12, cmd);
        memset(cmd, 0, sizeof(cmd));
        my_print(">", 2, 0);
    }
    fclose(fat12);
    return 0;
}
