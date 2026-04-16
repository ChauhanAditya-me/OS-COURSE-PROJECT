#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <cstring>
using namespace std;

char M[300][4];

char R[4];
char IR[4];
int IC;
int SI;
int PI;
int TI;
bool C;

int PTR[4];

struct PCB
{
    char jobId[5];
    int TTL;
    int TLL;
    int TTC;
    int LLC;
    int pageTableBlock;
    int pageTableLen;
};
PCB pcb;

bool frameUsed[30];

ifstream fin("input.txt");
ofstream fout("output.txt");

void LOAD();
void STARTEXECUTION();
void EXECUTEUSERPROGRAM();
void MOS();
void READ();
void WRITE();
void TERMINATE(int EM);
int ALLOCATE();
void ADDRESSMAP(int va, int &ra);
void DISPLAYMEMORY();
void clearPCB();

const char *errorMsg[] = {
    "No Error",
    "Out of Data",
    "Line Limit Exceeded",
    "Time Limit Exceeded",
    "Operation Code Error",
    "Operand Error",
    "Invalid Page Fault"};

void DISPLAYMEMORY()
{
    cout << "\033[2J\033[H";
    cout << "=== REAL MEMORY (300 words, 30 blocks) ===" << endl
         << endl;
    for (int i = 0; i < 300; i++)
    {
        cout << i << "\t|";
        for (int j = 0; j < 4; j++)
        {
            char ch = M[i][j];
            if (ch >= 32 && ch <= 126)
                cout << ch;
            else
                cout << ' ';
            cout << "|";
        }
        cout << endl;
    }
}

int ALLOCATE()
{
    int free[30], cnt = 0;
    for (int i = 0; i < 30; i++)
        if (!frameUsed[i])
            free[cnt++] = i;

    if (cnt == 0)
        return -1;

    int chosen = free[rand() % cnt];
    frameUsed[chosen] = true;
    return chosen;
}

void freeJobFrames()
{
    frameUsed[pcb.pageTableBlock] = false;

    int ptBase = pcb.pageTableBlock * 10;
    for (int p = 0; p < pcb.pageTableLen; p++)
    {
        int frameNo = M[ptBase + p][0] - '0';
        if (frameNo >= 0 && frameNo < 30)
            frameUsed[frameNo] = false;
    }
}

void clearPCB()
{
    pcb.TTL = pcb.TLL = pcb.TTC = pcb.LLC = 0;
    pcb.pageTableBlock = -1;
    pcb.pageTableLen = 0;
    for (int i = 0; i < 4; i++)
        pcb.jobId[i] = ' ';
    pcb.jobId[4] = '\0';
}

void ADDRESSMAP(int va, int &ra)
{
    int page = va / 10;
    int offset = va % 10;

    if (page > pcb.pageTableLen - 1)
    {
        PI = 2;
        ra = -1;
        return;
    }

    int ptBase = pcb.pageTableBlock * 10;
    int stored = (int)(unsigned char)M[ptBase + page][0];

    if (stored == 0 || stored == 0xFF)
    {
        PI = 3;
        ra = -1;
        return;
    }
    int frameNo = stored - 1;

    ra = frameNo * 10 + offset;
}

void TERMINATE(int EM)
{
    fout << endl
         << endl;
    fout << "Job: " << pcb.jobId << endl;
    fout << "Termination Status: " << errorMsg[EM] << endl;

    cout << "\n[TERMINATE] Job " << pcb.jobId
         << " | Status: " << errorMsg[EM] << endl;

    freeJobFrames();
    clearPCB();
    SI = TI = PI = 0;
}

void READ()
{
    string line;
    if (!getline(fin, line))
    {
        TERMINATE(1);
        return;
    }
    if (line.substr(0, 4) == "$END")
    {
        TERMINATE(1);
        return;
    }

    while ((int)line.size() < 40)
        line += ' ';

    int va = (IR[2] - '0') * 10 + (IR[3] - '0');
    int ra;
    int k = 0;
    for (int i = 0; i < 10; i++)
    {
        int word_va = va + i;
        ADDRESSMAP(word_va, ra);
        if (PI != 0)
        {
            MOS();
            return;
        }
        for (int j = 0; j < 4; j++)
            M[ra][j] = line[k++];
    }

    cout << "[GD] Data read into virtual block " << va / 10 << endl;
}

void WRITE()
{
    pcb.LLC++;
    if (pcb.LLC > pcb.TLL)
    {
        TERMINATE(2);
        return;
    }

    int va = (IR[2] - '0') * 10 + (IR[3] - '0');
    for (int i = 0; i < 10; i++)
    {
        int word_va = va + i;
        int ra;
        ADDRESSMAP(word_va, ra);
        if (PI != 0)
        {
            MOS();
            return;
        }
        for (int j = 0; j < 4; j++)
            fout << M[ra][j];
    }
    fout << endl;
    fout.flush();

    cout << "[PD] Output written from virtual block " << va / 10 << endl;
}

void MOS()
{
    if (TI == 2)
    {
        if (SI == 2)
        {
            WRITE();
        }
        TERMINATE(3);
        return;
    }

    if (PI != 0)
    {
        if (PI == 1)
            TERMINATE(4);
        else if (PI == 2)
            TERMINATE(5);
        else if (PI == 3)
        {
            TERMINATE(6);
        }
        PI = 0;
        return;
    }

    if (SI == 1)
        READ();
    else if (SI == 2)
        WRITE();
    else if (SI == 3)
        TERMINATE(0);

    SI = 0;
}

void EXECUTEUSERPROGRAM()
{
    while (true)
    {
        int ra_fetch;
        ADDRESSMAP(IC, ra_fetch);
        if (PI != 0)
        {
            MOS();
            if (pcb.pageTableBlock == -1)
                break;
            continue;
        }

        for (int i = 0; i < 4; i++)
            IR[i] = M[ra_fetch][i];
        IC++;

        int va_op = -1, ra_op = -1;
        bool needsOperand = !(IR[0] == 'H');

        if (needsOperand && IR[0] != 'B' && IR[0] != 'G' && IR[0] != 'P')
        {
            if (IR[2] < '0' || IR[2] > '9' || IR[3] < '0' || IR[3] > '9')
            {
                PI = 2;
                MOS();
                if (pcb.pageTableBlock == -1)
                    break;
                continue;
            }
            va_op = (IR[2] - '0') * 10 + (IR[3] - '0');
            ADDRESSMAP(va_op, ra_op);
            if (PI != 0)
            {
                MOS();
                if (pcb.pageTableBlock == -1)
                    break;
                continue;
            }
        }

        if (IR[0] == 'L' && IR[1] == 'R')
        {
            for (int i = 0; i < 4; i++)
                R[i] = M[ra_op][i];
        }
        else if (IR[0] == 'S' && IR[1] == 'R')
        {
            for (int i = 0; i < 4; i++)
                M[ra_op][i] = R[i];
        }
        else if (IR[0] == 'C' && IR[1] == 'R')
        {
            C = true;
            for (int i = 0; i < 4; i++)
                if (R[i] != M[ra_op][i])
                {
                    C = false;
                    break;
                }
        }
        else if (IR[0] == 'B' && IR[1] == 'T')
        {
            if (IR[2] < '0' || IR[2] > '9' || IR[3] < '0' || IR[3] > '9')
            {
                PI = 2;
                MOS();
                if (pcb.pageTableBlock == -1)
                    break;
                continue;
            }
            int target = (IR[2] - '0') * 10 + (IR[3] - '0');
            if (C)
                IC = target;
        }
        else if (IR[0] == 'B' && IR[1] == 'N')
        {
            if (IR[2] < '0' || IR[2] > '9' || IR[3] < '0' || IR[3] > '9')
            {
                PI = 2;
                MOS();
                if (pcb.pageTableBlock == -1)
                    break;
                continue;
            }
            int target = (IR[2] - '0') * 10 + (IR[3] - '0');
            if (!C)
                IC = target;
        }
        else if (IR[0] == 'G' && IR[1] == 'D')
        {
            SI = 1;
            MOS();
            if (pcb.pageTableBlock == -1)
                break;
        }
        else if (IR[0] == 'P' && IR[1] == 'D')
        {
            SI = 2;
            MOS();
            if (pcb.pageTableBlock == -1)
                break;
        }
        else if (IR[0] == 'H')
        {
            SI = 3;
            MOS();
            break;
        }
        else
        {
            PI = 1;
            MOS();
            if (pcb.pageTableBlock == -1)
                break;
        }

        pcb.TTC++;
        if (pcb.TTC >= pcb.TTL)
        {
            TI = 2;
            MOS();
            if (pcb.pageTableBlock == -1)
                break;
        }
    }
}

void STARTEXECUTION()
{
    IC = 0;
    SI = TI = PI = 0;
    pcb.TTC = pcb.LLC = 0;
    cout << "\n[START] Executing job " << pcb.jobId << endl;
    EXECUTEUSERPROGRAM();
}

void LOAD()
{
    string line;
    string pageBuf;
    int pageCount = 0;
    int ptBlock = -1;

    srand((unsigned)time(nullptr));

    cout << "MOS Stage II – Loading..." << endl;

    while (getline(fin, line))
    {
        if (line.substr(0, 4) == "$AMJ")
        {
            clearPCB();
            memset(M, 0, sizeof(M));
            memset(frameUsed, false, sizeof(frameUsed));

            for (int i = 0; i < 4; i++)
                pcb.jobId[i] = (i + 4 < (int)line.size()) ? line[4 + i] : ' ';
            pcb.jobId[4] = '\0';

            pcb.TTL = 0;
            pcb.TLL = 0;
            for (int i = 8; i < 12 && i < (int)line.size(); i++)
            {
                if (line[i] >= '0' && line[i] <= '9')
                    pcb.TTL = pcb.TTL * 10 + (line[i] - '0');
            }
            for (int i = 12; i < 16 && i < (int)line.size(); i++)
            {
                if (line[i] >= '0' && line[i] <= '9')
                    pcb.TLL = pcb.TLL * 10 + (line[i] - '0');
            }
            if (pcb.TTL == 0)
                pcb.TTL = 100;
            if (pcb.TLL == 0)
                pcb.TLL = 100;

            ptBlock = ALLOCATE();
            if (ptBlock == -1)
            {
                cerr << "ERROR: No frame for page table!" << endl;
                return;
            }
            pcb.pageTableBlock = ptBlock;
            int ptBase = ptBlock * 10;
            for (int i = 0; i < 10; i++)
                for (int j = 0; j < 4; j++)
                    M[ptBase + i][j] = (char)0xFF;

            pageCount = 0;
            pageBuf.clear();
            cout << "[AMJ] Job=" << pcb.jobId
                 << " TTL=" << pcb.TTL << " TLL=" << pcb.TLL
                 << " PageTableBlock=" << ptBlock << endl;
        }

        else if (line.substr(0, 4) == "$DTA")
        {
            if (!pageBuf.empty())
            {
                if (pageCount >= 10)
                {
                    cerr << "ERROR: Memory exceeded!" << endl;
                    TERMINATE(1);
                    return;
                }
                int frame = ALLOCATE();
                if (frame == -1)
                {
                    cerr << "ERROR: No free frame!" << endl;
                    TERMINATE(1);
                    return;
                }
                int ptBase = ptBlock * 10;
                M[ptBase + pageCount][0] = (char)(frame + 1);
                int frameBase = frame * 10;
                int k = 0;
                while ((int)pageBuf.size() < 40)
                    pageBuf += ' ';
                for (int i = 0; i < 10; i++)
                    for (int j = 0; j < 4; j++)
                        M[frameBase + i][j] = pageBuf[k++];
                cout << "[LOAD] Program page " << pageCount
                     << " → frame " << frame << " (final flush)" << endl;
                pageCount++;
                pageBuf.clear();
            }

            pcb.pageTableLen = pageCount;
            PTR[0] = 0;
            PTR[1] = pageCount - 1;
            PTR[2] = ptBlock / 10;
            PTR[3] = ptBlock % 10;

            cout << "[DTA] " << pageCount << " program pages loaded." << endl;
            STARTEXECUTION();
        }

        else if (line.substr(0, 4) == "$END")
        {
            cout << "[END] End of job " << pcb.jobId << "." << endl;
        }

        else
        {
            while (!line.empty() && (line.back() == '\r' || line.back() == '\n'))
                line.pop_back();

            bool isBlankLine = true;
            for (char ch : line)
                if (ch != ' ')
                {
                    isBlankLine = false;
                    break;
                }

            auto flushPage = [&](string pageData)
            {
                if (pageCount >= 10)
                {
                    cerr << "ERROR: Memory exceeded!" << endl;
                    return false;
                }
                int frame = ALLOCATE();
                if (frame == -1)
                {
                    cerr << "ERROR: No free frame!" << endl;
                    return false;
                }
                int ptBase = ptBlock * 10;
                M[ptBase + pageCount][0] = (char)(frame + 1);
                int frameBase = frame * 10;
                int k = 0;
                while ((int)pageData.size() < 40)
                    pageData += ' ';
                for (int i = 0; i < 10; i++)
                    for (int j = 0; j < 4; j++)
                        M[frameBase + i][j] = pageData[k++];
                cout << "[LOAD] Program page " << pageCount
                     << " → frame " << frame << endl;
                pageCount++;
                return true;
            };

            if (isBlankLine)
            {
                if (!pageBuf.empty())
                {
                    if (!flushPage(pageBuf))
                    {
                        TERMINATE(1);
                        return;
                    }
                    pageBuf.clear();
                }
                if (!flushPage(string(40, ' ')))
                {
                    TERMINATE(1);
                    return;
                }
            }
            else
            {
                pageBuf += line;

                while ((int)pageBuf.size() >= 40)
                {
                    if (!flushPage(pageBuf.substr(0, 40)))
                    {
                        TERMINATE(1);
                        return;
                    }
                    pageBuf = pageBuf.substr(40);
                }
            }
        }
    }
}

int main()
{
    if (!fin.is_open())
    {
        cerr << "Cannot open input file!" << endl;
        return 1;
    }
    LOAD();
    cout << "\nDone. Check output.txt for results." << endl;
    return 0;
}
