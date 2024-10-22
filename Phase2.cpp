#include <iostream>
#include <fstream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <cctype>

using namespace std;

char M[300][4], IR[4], GR[4];
bool C;
int PTR;

struct PCB {
    char job[4], TTL[4], TLL[4];
} pcb;

int VA, RA, TTC, LLC, TTL, TLL, EM, SI, TI, PI, ttl, tll, IC, pte, InValid = 0;
fstream fin, fout;
string line;
int pageAllocated[30];

void initialization();
void load();
void Pagetable();
void allocate();
void startExecution();
void executeProgram();
void AddMap();
void Examine();
void MOS();
void Terminate();
void read();
void write();

void initialization() {
    SI = TI = PI = TTC = LLC = TTL = TLL = EM = VA = RA = IC = PTR = InValid = 0;
    for (int i = 0; i < 30; i++) {
        pageAllocated[i] = 0;
    }
    for (int i = 0; i < 4; i++) {
        IR[i] = '&';
        GR[i] = '_';
    }
    for (int i = 0; i < 300; i++) {
        for (int j = 0; j < 4; j++) {
            M[i][j] = '_';
        }
    }
}

void load() {
    while (getline(fin, line)) {
        if (line.substr(0, 4) == "$AMJ") {
            initialization();
            copy(line.begin() + 4, line.begin() + 8, pcb.job);
            copy(line.begin() + 8, line.begin() + 12, pcb.TTL);
            copy(line.begin() + 12, line.begin() + 16, pcb.TLL);
            ttl = stoi(line.substr(8, 4));
            tll = stoi(line.substr(12, 4));
            Pagetable();
            allocate();
            cout << "Loaded job: " << string(pcb.job, pcb.job + 4) << " with TTL: " << ttl << " and TLL: " << tll << endl;

        } else if (line.substr(0, 4) == "$DTA") {
            startExecution();
        }
    }
}

void Pagetable() {
    PTR = (rand() % 29) * 10;
    pageAllocated[PTR / 10] = 1;

    for (int i = PTR; i < PTR + 10; i++) {
        fill(begin(M[i]), end(M[i]), '*');
    }
}

void allocate() {
    bool check = false;
    int pos;
    while (!check) {
        pos = (rand() % 29) * 10;
        while (pageAllocated[pos / 10] != 0) {
            pos = (rand() % 29) * 10;
        }
        pageAllocated[pos / 10] = 1;

        string str = to_string(pos);
        if (pos < 100) {
            M[PTR][2] = '0';
            M[PTR][3] = str[0];
        } else {
            M[PTR][2] = str[0];
            M[PTR][3] = str[1];
        }

        getline(fin, line);
        int k = 0;
        for (int i = 0; i < line.size() / 4; i++) {
            for (int j = 0; j < 4; j++) {
                M[pos + i][j] = line[k++];
                if (line[k] == 'H') {
                    check = true;
                    M[pos + i + 1][0] = 'H';
                    M[pos + i + 1][1] = '0';
                    M[pos + i + 1][2] = '0';
                    M[pos + i + 1][3] = '0';
                }
            }
        }
    }
}

void startExecution() {
    IC = 0;
    executeProgram();
}

void executeProgram() {
    int no = stoi(string(1, M[PTR][2]) + M[PTR][3]);
    while (IR[0] != 'H') {
        cout << "Executing at IC: " << IC << ", IR: " << string(IR, IR + 4) << endl;
        copy(begin(M[no * 10 + IC]), end(M[no * 10 + IC]), IR);
        if (!isdigit(IR[2]) || !isdigit(IR[3])) {
            PI = 2;
            TI = (TTC >= ttl) ? 2 : 0;
            MOS();
        }
        VA = (IR[2] - '0') * 10 + (IR[3] - '0');
        AddMap();
        Examine();
    }
}

void AddMap() {
    pte = PTR + (VA / 10);
    if (M[pte][3] == '*') {
        PI = 3;
        EM = 7;
        int pos = (rand() % 29) * 10;
        while (pageAllocated[pos / 10] != 0) {
            pos = (rand() % 29) * 10;
        }
        pageAllocated[pos / 10] = 1;

        string str = to_string(pos);
        if (pos < 100) {
            M[pte][2] = '0';
            M[pte][3] = str[0];
        } else {
            M[pte][2] = str[0];
            M[pte][3] = str[1];
        }
    } else {
        PI = 0;
    }
    int p = (M[pte][2] - '0') * 10 + (M[pte][3] - '0');
    RA = p * 10 + (VA % 10);
    if (RA > 300) {
        PI = 2;
        TI = 0;
        MOS();
    }
}

void Examine() {
    if (IR[0] == 'G') { // 'GD' for Read
        IC++;
        if (IR[1] == 'D') {
            SI = 1; // Set interrupt for reading data
            TTC += 2; // Increment Time Taken Counter for GD
            TI = (TTC < ttl) ? 0 : 2; // Check if time limit exceeded
            MOS();
        } else {
            PI = 1; // Set interrupt for invalid operation code
            TI = (TTC >= ttl) ? 2 : 0; // Check if time limit exceeded
            MOS();
        }
    } 
    else if (IR[0] == 'P') { // 'PD' for Write
        IC++;
        if (IR[1] == 'D') {
            LLC++;
            TTC++;
            SI = 2; // Set interrupt for writing data
            TI = (TTC < ttl) ? 0 : 2; // Check if time limit exceeded
            MOS();
        } else {
            PI = 1; // Set interrupt for invalid operation code
            TI = (TTC >= ttl) ? 2 : 0; // Check if time limit exceeded
            MOS();
        }
    } 
    else if (IR[0] == 'L') { // 'LR' for Load Register
        IC++;
        if (IR[1] == 'R') {
            if (PI == 3) {
                InValid = 1; // Mark as invalid page fault
                TI = 0;
                MOS();
            } else {
                copy(begin(M[RA]), end(M[RA]), GR); // Load data from memory to GR
                TTC++;
            }
            if (TTC > ttl) {
                PI = 3;
                TI = 2;
                MOS();
            }
        } else {
            PI = 1; // Invalid operation
            TI = (TTC >= ttl) ? 2 : 0;
            MOS();
        }
    } 
    else if (IR[0] == 'S') { // 'SR' for Store Register
        IC++;
        if (IR[1] == 'R') {
            copy(begin(GR), end(GR), M[RA]); // Store data from GR to memory
            TTC += 2;
            if (TTC > ttl) {
                TI = 2;
                PI = 3;
                MOS();
            }
        } else {
            PI = 1; // Invalid operation
            TI = (TTC >= ttl) ? 2 : 0;
            MOS();
        }
    } 
    else if (IR[0] == 'C') { // 'CR' for Compare Register
        IC++;
        if (IR[1] == 'R') {
            if (PI == 3) {
                InValid = 1;
                TI = 0;
                MOS();
            } else {
                C = equal(begin(GR), end(GR), begin(M[RA])); // Compare GR with memory
                TTC++;
            }
            if (TTC > ttl) {
                TI = 2;
                PI = 3;
                MOS();
            }
        } else {
            PI = 1; // Invalid operation
            TI = (TTC >= ttl) ? 2 : 0;
            MOS();
        }
    } 
    else if (IR[0] == 'B') { // 'BT' for Branch if True
        IC++;
        if (IR[1] == 'T') {
            if (PI == 3) {
                InValid = 1;
                TI = 0;
                MOS();
            } else if (C) {
                IC = VA; // Update IC if condition is true
                TTC++;
            }
            if (TTC > ttl) {
                TI = 2;
                PI = 3;
                MOS();
            }
        } else {
            PI = 1; // Invalid operation
            TI = (TTC >= ttl) ? 2 : 0;
            MOS();
        }
    } 
    else if (IR[0] == 'H') { // 'H' for Halt
        IC++;
        TTC++;
        if (TTC > ttl) {
            TI = 2;
            PI = 3;
            MOS();
        } else {
            SI = 3; // Set interrupt for normal termination
            MOS();
        }
    } 
    else {
        PI = 1; // Undefined opcode
        TI = (TTC > ttl) ? 2 : 0;
        MOS();
    }
}

void MOS() {
    if (PI == 1) {
        if (TI == 0) {
            EM = 4;
            cout << "Opcode Error" << endl;
            Terminate();
        } else if (TI == 2) {
            EM = 3;
            cout << "Time Limit Exceeded" << endl;
            Terminate();
        }
    } else if (SI == 1) {
        if (TI == 0) {
            read();
        } else if (TI == 2) {
            EM = 3;
            cout << "Time Limit Exceeded" << endl;
            Terminate();
        }
    }
    if (SI == 3) {
        EM = 0;
        cout << "No Error" << endl;
        Terminate();
    }
}

void Terminate() {
    fout << "Job ID: " << string(pcb.job, pcb.job + 4) << "\n";
    fout << "TTL = " << ttl << ", TLL = " << tll << ", TTC = " << TTC << ", LLC = " << LLC << endl;
    fout << "PTR = " << PTR << ", IC = " << IC << ", EM = " << EM << endl;
    load();
}

void read() {
    getline(fin, line);
    if (line.substr(0, 4) == "$END") {
        EM = 1;
        cout << "Out of Data" << endl;
        Terminate();
    }
    for (int i = 0; i < line.size(); i += 4) {
        for (int j = 0; j < 4 && i + j < line.size(); j++) {
            M[RA + i / 4][j] = line[i + j];
        }
    }
}

void write() {
    string buffer;
    int ra = RA;
    while (true) {
        for (int i = 0; i < 4; i++) {
            if (M[ra][i] == '_') break;
            buffer += M[ra][i];
        }
        ra++;
    }
    fout << buffer << endl;
}

int main() {
    srand(time(0));
    fin.open("input_2.txt", ios::in);
    fout.open("output_2.txt", ios::out);
    load();
    fin.close();
    fout.close();
    return 0;
}