#include <bits/stdc++.h>
using namespace std;

class OS
{
private:
    char M[100][4];
    char IR[4];
    char R[4];
    int IC;
    bool C;
    int SI;
    char buffer[100];

public:
    fstream fin, fout;

    void INIT();
    void LOAD();
    void START();
    void EXECUTE();
    void MOS();

    int ADDR();

    void READ();
    void WRITE();
    void HALT();
};

// ---------------- INIT ----------------
void OS::INIT()
{
    for (int i = 0; i < 100; i++)
        for (int j = 0; j < 4; j++)
            M[i][j] = ' ';

    IC = 0;
    C = false;
}

// ---------------- LOAD ----------------
void OS::LOAD()
{
    string line;
    int mem_ptr = 0;

    while (getline(fin, line))
    {
        if (line.substr(0, 4) == "$AMJ")
        {
            INIT();
            mem_ptr = 0;
        }
        else if (line.substr(0, 4) == "$DTA")
        {
            START(); // start execution AFTER program loaded
        }
        else if (line.substr(0, 4) == "$END")
        {
            continue;
        }
        else
        {
            // Load instructions only
            if (line[0] != '$')
            {
                int k = 0;
                while (k < line.size())
                {
                    if (line[k] == 'H')
                    {
                        M[mem_ptr][0] = line[k++];
                        for (int j = 1; j < 4; j++)
                            M[mem_ptr][j] = ' ';
                        mem_ptr++;
                        continue;
                    }

                    for (int j = 0; j < 4; j++)
                    {
                        if (k < line.size())
                            M[mem_ptr][j] = line[k++];
                        else
                            M[mem_ptr][j] = ' ';
                    }
                    mem_ptr++;
                }
            }
        }
    }
}

// ---------------- ADDRESS ----------------
int OS::ADDR()
{
    if (IR[2] == ' ' || IR[3] == ' ')
        return 0;
    return (IR[2] - '0') * 10 + (IR[3] - '0');
}

// ---------------- START ----------------
void OS::START()
{
    IC = 0;
    EXECUTE();
}

// ---------------- EXECUTE ----------------
void OS::EXECUTE()
{
    while (true)
    {
        for (int i = 0; i < 4; i++)
            IR[i] = M[IC][i];

        IC++;

        int loc = ADDR();

        if (IR[0] == 'G' && IR[1] == 'D')
        {
            SI = 1;
            MOS();
        }
        else if (IR[0] == 'P' && IR[1] == 'D')
        {
            SI = 2;
            MOS();
        }
        else if (IR[0] == 'H')
        {
            SI = 3;
            MOS();
            break;
        }
        else if (IR[0] == 'L' && IR[1] == 'R')
        {
            for (int i = 0; i < 4; i++)
                R[i] = M[loc][i];
        }
        else if (IR[0] == 'S' && IR[1] == 'R')
        {
            for (int i = 0; i < 4; i++)
                M[loc][i] = R[i];
        }
        else if (IR[0] == 'C' && IR[1] == 'R')
        {
            C = true;
            for (int i = 0; i < 4; i++)
            {
                if (R[i] != M[loc][i])
                {
                    C = false;
                    break;
                }
            }
        }
        else if (IR[0] == 'B' && IR[1] == 'T')
        {
            if (C == true)
                IC = loc;
        }
    }
}

// ---------------- MOS ----------------
void OS::MOS()
{
    if (SI == 1)
        READ();
    else if (SI == 2)
        WRITE();
    else if (SI == 3)
        HALT();
}

// ---------------- READ ----------------
void OS::READ()
{
    // skip control cards and blank lines
    do
    {
        if (!fin.getline(buffer, 100))
            return;
    } while (buffer[0] == '$' || buffer[0] == '\0');

    int loc = ADDR();
    int k = 0;

    for (int i = 0; i < 10; i++)
    {
        for (int j = 0; j < 4; j++)
        {
            if (buffer[k] != '\0')
                M[loc][j] = buffer[k++];
            else
                M[loc][j] = ' ';
        }

        if (buffer[k] == '\0')
            break;

        loc++;
    }
}

// ---------------- WRITE ----------------
void OS::WRITE()
{
    int loc = ADDR();
    string line = "";

    for (int i = 0; i < 10; i++)
    {
        string word = "";

        for (int j = 0; j < 4; j++)
            word += M[loc][j];

        if (word == "    ")
            break;

        line += word;
        loc++;
    }

    while (!line.empty() && line.back() == ' ')
        line.pop_back();

    fout << line << endl;
}

// ---------------- HALT ----------------
void OS::HALT()
{
    fout << endl;
}

// ---------------- MAIN ----------------
int main()
{
    OS os;

    os.fin.open("input1.txt");
    os.fout.open("output1.txt", ios::out | ios::trunc);

    if (!os.fin)
    {
        cout << "Input file not found\n";
        return 0;
    }

    if (!os.fout)
    {
        cout << "Failed to create output file\n";
        return 0;
    }

    os.LOAD();

    os.fin.close();
    os.fout.close();

    cout << "Execution Complete. Check output1.txt\n";
}
