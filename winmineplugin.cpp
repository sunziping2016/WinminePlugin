#include <stdio.h> // Using C's IO for smaller .exe file size
#include <assert.h>
#include <windows.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

const unsigned int MAXLINEBYTE=100, MAXFILENAMEBYTE=25;

int timedelay=0; // Delay several milliseconds after displaying a sentence to make debug easier.
enum { SOLVEMINE=0, SOLVEALL=1};
int solvemode=SOLVEMINE; // Decide whether to use MineMap::SolveAllButReserveSeveral or MineMap::SolveMineButReserveSeveral
int reserve=0; // Decide what number will be passed to the MineMap::Solve****ButReserveSeveral
bool waitexit=true, createprocess=false;

const char * const inifilename="winmineplugin.ini"; // The whole file name of the .ini

char logfilename[MAXFILENAMEBYTE]="";

FILE *logfile=NULL, *inifile=NULL;

class MineMap
{
public:
    MineMap() // You can use "=" to assign later.
     : map(0), sizex(0), sizey(0) {}
    MineMap(const unsigned char *buffer)
     : map(new int*[16]), sizex(0), sizey(0) // Extract the 2-D array from the orignal data in winmine.
    {                                        // Use dynamic memory. It may be hard to understand the code.
        ++buffer;
        for(; buffer[0]!='\x10'&&sizey<16; buffer+=block, ++sizey)
        {
            size_t end;
            for(end=block-2; buffer[end]!='\x10'; --end);
            assert(end>0&&(sizex==0||end==sizex));
            sizex=end;
            map[sizey]=new int[sizex];
            for(size_t i=0; i<sizex; ++i)
                map[sizey][i]=buffer[i];
        }
    }
    MineMap(const MineMap &orig) // Copy Ctor
     : sizex(orig.sizex), sizey(orig.sizey)
    {
        size_t i, j;
        map=new int *[sizey];
        for(i=0; i<sizey; ++i){
            map[i]=new int[sizex];
            for(j=0; j<sizex; ++j)
                map[i][j]=orig.map[i][j];
        }
    }
    MineMap & operator = (const MineMap &orig) // Copy operator
    {
        if(map){
            for(size_t i=0; i<sizey; ++i)
                delete []map[i];
            delete []map;
        }
        sizex=orig.sizex;
        sizey=orig.sizey;
        size_t i, j;
        map=new int *[sizey];
        for(i=0; i<sizey; ++i){
            map[i]=new int[sizex];
            for(j=0; j<sizex; ++j)
                map[i][j]=orig.map[i][j];
        }
        return *this;
    }
    void DrawMap() const
    {
        size_t i;
        printf("\t┌"); // The strings in this function is used to draw a table. It may be shown abnormally in this ASCII document.
        for(i=0; i<sizex; ++i)
            printf("──"); // Not using putchar to avoid international character problem.
        printf("┐\n");
        for(i=0; i<sizey; ++i){
            printf("\t│");
            for(size_t j=0; j<sizex; ++j){
                if(map[i][j]==0xCC) // Client has clicked the area which contains a mine. And of cource, game is over.
                    putchar('!');
                else if(map[i][j]>>7==1) // Area which contains mine.
                    putchar('*');
                else if((map[i][j]&0x0F)==0x0F) // Area which has not been discovered by client and without mine.
                    putchar('.');
                else if((map[i][j]&0x0F)==0x0E) // Area which is wrongly marked hasing mine by client. winmine would display a flag on the area.
                    putchar('#');
                else if((map[i][j]&0x0F)==0x0D) // Area which is marked '?' by client and without mine. winmine would display a '?' on the area
                    putchar('?');
                else if(map[i][j]==0x40)
                    putchar(' '); // Area which has been discovered by client and not surrounded by mine.
                else
                    putchar(map[i][j]-0x40+'0'); // Area which has been discovered by client and surrounded by mines. Display the surrounding mine's amount.
                putchar(' ');
            }
            printf("│\n");
        }
        printf("\t└");
        for(i=0; i<sizex; ++i)
            printf("──");
        printf("┘\n");
    }
    ~MineMap()
    {
        if(map){
            for(size_t i=0; i<sizey; ++i)
                delete []map[i];
            delete []map;
        }
    }
    size_t getHeight() const
    {
        return sizey;
    }
    size_t getWidth() const
    {
        return sizex;
    }
    bool IsStarted() const // Game has not started
    {
        size_t i, j;
        for(i=0; i<sizey; ++i)
            for(j=0; j<sizex; ++j)
                if((map[i][j]&0x0F)!=0x0F)
                    return true;
        return false;
    }
    bool IsFinished() const // Game is over.
    {
        bool flag=true; // Assume client has discovered all the areas.
        size_t i, j;
        for(i=0; i<sizey; ++i)
            for(j=0; j<sizex; ++j){
                if(map[i][j]==0xCC) // Client lose.
                    return true;
                if((map[i][j]&0x0F)==0x0F) // An area has not been discovered by client.
                    flag=false;
            }
        return flag;
    }
    bool IsPlaying() const
    {
        return IsStarted()&&!IsFinished();
    }
    bool IsHasingMine(size_t i, size_t j)
    {
        return map[i][j]>>7;
    }
    MineMap SolveAllButReserveSeveral(int reserve) const // Notice: It's const member fucntion
    {                                                    // This function will RETURN a solved MineMap generated by current MineMap
        MineMap solution;                                // And it will also reserve several area without mine to leave it to the client.
        solution.sizex=sizex;                            // HERE IS A BUG
        solution.sizey=sizey;                            // IF YOU USE THIS FUNCTION TO CHEAT, THE WINMINE WILL NOT STOPPED EVEN IF YOU FINISH
        solution.map=new int *[sizey];                   // DICOVERING ALL THE AREAS!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        size_t i, j;
        for(i=0; i<sizey; ++i){
            solution.map[i]=new int[sizex];
            for(j=0; j<sizex; ++j){
                if(map[i][j]>>7==1){ // Contain a mine
                    solution.map[i][j]=0x8E; // An area marked hasing mine
                    continue;
                }
                if(reserve>0&&reserve--){
                    solution.map[i][j]=0x0F; // An area has not been discovered.
                    continue;
                }
                int count=0; // Calculate the surrounding mine's amount.
                if(i!=0){
                    if(j!=0)
                        count+=map[i-1][j-1]>>7;
                    if(j!=sizex-1)
                        count+=map[i-1][j+1]>>7;
                    count+=map[i-1][j]>>7;
                }
                if(i!=sizey-1){
                    if(j!=0)
                        count+=map[i+1][j-1]>>7;
                    if(j!=sizex-1)
                        count+=map[i+1][j+1]>>7;
                    count+=map[i+1][j]>>7;
                }
                if(j!=0)
                    count+=map[i][j-1]>>7;
                if(j!=sizex-1)
                    count+=map[i][j+1]>>7;
                solution.map[i][j]=0x40+count;
            }
        }
        return solution;
    }
    MineMap SolveMineButReserveSeveral(int reserve) const // This function  will return a solved MineMap generated by current MineMap
    {                                                      // And it will only Solve areas which contain mine.
        MineMap solution;                                  // And it will also reserve several area with mine to leave it to the client.
        solution.sizex=sizex;
        solution.sizey=sizey;
        solution.map=new int *[sizey];
        size_t i, j;
        for(i=0; i<sizey; ++i){
            solution.map[i]=new int[sizex];
            for(j=0; j<sizex; ++j){
                if(map[i][j]>>7==1){
                    if(reserve>0&&reserve--)
                        solution.map[i][j]=0x8F;
                    else
                        solution.map[i][j]=0x8E;
                }
                else if((map[i][j]&0x0F)==0x0E)
                    solution.map[i][j]=0x0F;
                else
                    solution.map[i][j]=map[i][j];
            }
        }
        return solution;
    }
    void TransferDataToBuffer(unsigned char *buffer, unsigned char *end) const // May be hard to understand. Just write the 2-D array back to the orignal data buffer.
    {
        memset(buffer, 0x0F, end-buffer);
        size_t i, j;
        for(i=0; i<sizey; ++i){
            for(j=0; j<block; ++j){
                if(j>sizex+1)
                    buffer[j]=0x0F;
                else if(j==0 || j==sizex+1)
                    buffer[j]=0x10;
                else
                    buffer[j]=map[i][j-1];
            }
            buffer+=block;
        }
        if(buffer+block>end)
            return;
        for(i=0; i<sizex+2; ++i)
            buffer[i]=0x10;
    }
    int MarkedCount() const // Return the amount of the areas marked hasing mine.
    {
        int count=0;
        for(size_t i=0; i<sizey; ++i)
            for(size_t j=0; j<sizex; ++j)
                if((map[i][j]&0x0F)==0x0E)
                    ++count;
        return count;
    }
    int MineCount() const // Return the amount of the areas hasing mine.
    {
        int count=0;
        for(size_t i=0; i<sizey; ++i)
            for(size_t j=0; j<sizex; ++j)
                if(map[i][j]>>7==1)
                    ++count;
        return count;
    }
private:
    int **map; // A 2-D array to store the data, using dynamic memory;
    size_t sizex, sizey; // sizex is the width, and sizey is the height
    static const size_t block=32; // It's the memory block in the
};


// Several functions for output text.
void InformExit(int i=1)
{
    exit(i);
}

void InformResult(bool flag, const char *succeed, const char *fail)
{
    if(!flag){
        printf("[FAIL]%s\n", fail);
        InformExit();
    }
    else
        printf("[OKAY]%s\n", succeed);
    Sleep(timedelay);
}

void InformText(const char *info)
{
    printf("[INFO]%s\n",info);
    Sleep(timedelay);
}

void InformTitle(const char *title)
{
    printf("****************%s****************\n", title);
    Sleep(timedelay);
}

void OutputBuffer(unsigned char *buffer, unsigned char *end) // Used to debug. If you want to watch the real buffer, use this funcion
{
    for(size_t i=0; buffer+i<end; ++i){
        if(i%8==0)
            putchar('\t');
        printf("%02X ", buffer[i]);
        if((i+1)%8==0)
            putchar('\n');
    }
}

char filebuffer[MAXLINEBYTE]="";

bool ReadLine(FILE *file)
{
    int ch;
    size_t i=0;
    while(1){
        ch=fgetc(file);
        if(ch==EOF){
            if(i==0)
                return false;
            else
                break;
        }
        if(ch=='\n'){
            if(i==0)
                continue;
            else
                break;
        }
        if(isspace(ch)){
            if(i!=0)
                filebuffer[i]=ch;
            else
                continue;
        }
        else if(ch==';'){ // Comment in the ini file
            if(i!=0){
                while((ch=fgetc(file))!=EOF&&ch!='\n');
                break;
            }
            else{
                while((ch=fgetc(file))!=EOF&&ch!='\n');
                continue;
            }
        }
        else
            filebuffer[i]=ch;
        if(++i==MAXLINEBYTE){ // Avoid buffer overflow
            printf("[FAIL]No enough buffer for ini file.\n");
            InformExit();
        }
    }
    while(isspace(filebuffer[--i])); //Delete the following space.
    filebuffer[i+1]='\0';
    return true;
}

int GetNumber(const char *str)
{
    int i=0;
    for(i=0; str[i]!='\0'; ++i){
        if(!isdigit(str[i])){
            printf("[FAIL]Bad format in ini file. A non-digital character in a number.\n");
            InformExit();
        }
        if(i==9){
            printf("[FAIL]Bad format in ini file. A number to long.\n");
            InformExit();
        }
    }
    return atoi(str);
}

bool GetBool(const char *str)
{
    if(!strcmp(str, "true"))
        return true;
    else if(!strcmp(str, "false"))
        return false;
    printf("[FAIL]Bad format in ini file. Invalid bool type.\n");
    InformExit();
    return true; // Avoid warning
}

bool LoadIni()
{
    if(!(inifile=fopen(inifilename, "r")))
        return false;
    if(!(ReadLine(inifile)&&!strcmp(filebuffer,"[setting]"))){
        printf("[FAIL]Bad format in ini file. No \"[setting]\" section.\n");
        InformExit();
        return false;
    }
    while(ReadLine(inifile)){
        int i=strlen(filebuffer)+1, j;
        while(filebuffer[--i]!='=')
            if(i==0){
                printf("[FAIL]Bad format in ini file. Key without \"=\" .\n");
                InformExit();
            }
        if(i==0){
            printf("[FAIL]Bad format in ini file. Key without name.\n");
            InformExit();
        }
        j=i;
        while(isspace(filebuffer[--j]));
        filebuffer[++j]='\0';
        do
            ++i;
        while(isspace(filebuffer[i]));
        if(filebuffer[i]=='\0'){
            printf("[FAIL]Bad format in ini file. Key without value.\n");
            InformExit();
        }
        if(!strcmp(filebuffer, "timedelay")){
            timedelay=GetNumber(filebuffer+i);
        }
        else if(!strcmp(filebuffer, "solvemode")){
            if(!strcmp(filebuffer+i, "SOLVEMINE")){
                solvemode=SOLVEMINE;
            }
            else if(!strcmp(filebuffer+i, "SOLVEALL")){
                solvemode=SOLVEALL;
            }
            else{
                printf("[FAIL]Bad format in ini file. Unknown solvemode.\n");
                InformExit();
            }
        }
        else if(!strcmp(filebuffer, "reserve")){
            reserve=GetNumber(filebuffer+i);
        }
        else if(!strcmp(filebuffer, "waitexit")){
            if(!strcmp(filebuffer+i, "true")){
                waitexit=true;
            }
            else if(!strcmp(filebuffer+i, "false")){
                waitexit=false;
            }
            else{
                printf("[FAIL]Bad format in ini file. Invalid bool type.\n");
                InformExit();
            }
        }
        else if(!strcmp(filebuffer, "logfile")){
            if(strlen(filebuffer+i)<MAXFILENAMEBYTE){
                strcpy(logfilename, filebuffer+i);
            }
            else{
                printf("[FAIL]Bad format in ini file. File name too long.\n");
                InformExit();
            }
        }
        else{
            printf("[FAIL]Bad format in ini file. Unkownn key type.\n");
            InformExit();
        }
    }
    fclose(inifile);
    return true;
}

void CreateIni() // Used to debug. Create a default ini file
{
    const char * const defaultinitext= //The default text of the ini file.
        "[setting]\n"
        "timedelay=0\n"
        "solvemode=SOLVEMINE\n"
        "reserve=0\n"
        "waitexit=true\n"
        "logfile=winmineplugin.txt\n";
    FILE *inifile;
    if(!(inifile=fopen(inifilename, "w")))
        return;
    fprintf(inifile, defaultinitext);
    fclose(inifile);
}

int main()
{
    BOOL ret;
    InformTitle("WELCOME");
    InformText("Trying to load the ini file.");
    if(LoadIni()){
        printf("[OKAY]Succeed to load the ini file.\n");
        printf("\ttimedelay=%d\n", timedelay);
        switch(solvemode)
        {
        case SOLVEMINE:
            printf("\tsolvemode=SOLVEMINE\n");
            break;
        case SOLVEALL:
            printf("\tsolvemode=SOLVEALL\n");
            break;
        }
        printf("\treserve=%d\n", reserve);
        if(waitexit)
            printf("\twaitexit=true\n");
        else
            printf("\twaitexit=false\n");
        if(strlen(logfilename))
            printf("\tlogfile=%s", logfilename);
    }
    else
        printf("[FAIL]Have not found the ini file.\n");
    Sleep(timedelay);
    const char *title="Minesweeper"; //This string is the frame title of winmine. It may be shown abnormally in this ASCII document.
    printf("[INFO]Trying to use the title \"%s\" to get the window handle.\n", title);
    Sleep(timedelay);
    HWND hWnd=FindWindow(NULL, title);
    if(!hWnd){
        printf("[FAIL]Have not found the window.\n");
        InformExit();
    }
    else
        printf("[OKAY]Get the window handle: 0x%p .\n", hWnd);
    Sleep(timedelay);
    InformText("Trying to get the PID.");
    DWORD dwPID;
    GetWindowThreadProcessId(hWnd, &dwPID);
    if(!dwPID){
        printf("[FAIL]Cannot get the PID.\n");
        InformExit();
    }
    else{
        printf("[OKAY]Get the PID: %lu .\n", dwPID);
    }
    Sleep(timedelay);
    InformText("Trying to get the process handle.");
    HANDLE hProcess=OpenProcess(PROCESS_ALL_ACCESS, FALSE, dwPID);
    if(!hProcess){
        printf("[FAIL]Cannot get the process handle.\n");
        InformExit();
    }
    else
        printf("[OKAY]Get the process handle: 0x%p .\n", hProcess);
    Sleep(timedelay);
    const DWORD start=0x01005360, end=0x01005560, size=end-start; // The address of the 2-D array in winmine.
    const DWORD pcount=0x01005194; // The address of the count in winmine. It will display on the upper left corner on the winmine frame.
    printf("[INFO]Trying to get the data from 0x%p to 0x%p .\n", (void *)start, (void *)end);
    Sleep(timedelay);
    DWORD rsize;
    unsigned char buffer[size];
    ret=ReadProcessMemory(hProcess, (void *)start, buffer, size, &rsize);
    InformResult(ret&&size==rsize, "Succeed to get the data.", "Fail to read the data.");
    InformText("Now analysing the data");
    MineMap map(buffer), solutionmap;
    printf("[OKAY]Succeed to analyse the data.\n\tThe size is %d*%d.\n", map.getWidth(), map.getHeight());
    printf("\tHere are the map:\n");
    map.DrawMap();
    Sleep(timedelay);
    InformText("Now generate the key.");
    switch(solvemode)
    {
    case SOLVEMINE:
        solutionmap=map.SolveMineButReserveSeveral(reserve);
        break;
    case SOLVEALL:
        solutionmap=map.SolveAllButReserveSeveral(reserve);
        break;
    default:
        break;
    }
    printf("[OKAY]Succeed to generate the key. The key map is:\n");
    Sleep(timedelay);
    solutionmap.DrawMap();
    if(!map.IsStarted()){
        printf("[FAIL]Game has not started yet.\n");
        InformExit();
    }
    if(map.IsFinished()){
        printf("[FAIL]Game has already finished.\n");
        InformExit();
    }
    Sleep(timedelay);
    solutionmap.TransferDataToBuffer(buffer, buffer+size);
    InformText("Trying to write the data back to the process.");
    ret=WriteProcessMemory(hProcess, (void *)start, buffer, size, &rsize)&&size==rsize;
    unsigned char count=solutionmap.MineCount()-solutionmap.MarkedCount();
    ret=ret&&WriteProcessMemory(hProcess, (void *)pcount, &count, 1, &rsize)&&1==rsize;
    CloseHandle(hProcess);
    InformResult(ret, "Succeed to write the data.", "Fail to write the data.");
    printf("[INFO]Now refresh the window.\n");
    Sleep(timedelay);
    // Bring the winmine's frame to front. And refresh it.
    InformResult(ShowWindow(hWnd, SW_MINIMIZE)&&ShowWindow(hWnd, SW_NORMAL), "Succeed to refresh the window.", "Fail to refresh the window.");
    for(size_t i=0; i<solutionmap.getHeight(); ++i)
        for(size_t j=0; j<solutionmap.getWidth(); ++j)
            if(!solutionmap.IsHasingMine(i, j)){
                PostMessage(hWnd, WM_LBUTTONDOWN, 0, MAKELONG((j)*16+0xC, (i)*16+0x37));
                PostMessage(hWnd, WM_LBUTTONUP, 0, MAKELONG((j)*16+0xC, (i)*16+0x37));
            }
    InformTitle("SUCCEED");
    InformExit(0);
}
