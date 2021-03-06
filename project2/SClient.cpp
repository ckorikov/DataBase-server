#include "SClient.h"
#include <iostream>        
#include <thread>          
#include <string>
#include <regex>
#include <map>
#include <fstream>
#include <stdio.h>
#include <ctime>
#define DEFAULT_BUFFER_LENGTH 256



using namespace std;

typedef map<string, int>  mapT;
mapT tables;


int save_tables() {
    string fname = "Tables.bin";
    FILE* fp = fopen(fname.c_str(), "w");
    if (!fp)
        return -errno;
    mapT::iterator it = tables.begin();
    for (int i = 0; it != tables.end(); it++, i++) {
        fprintf(fp, "%s=%s\n", it->first.c_str(), to_string(it->second).c_str());
    }
    fclose(fp);
    return 0;
}

int read_tables() {

    string fname = "Tables.bin";

    ifstream input(fname);
    string line;


    tables.clear();

    char* buf = 0;
    size_t buflen = 0;

    regex wrd("\\w+");
    while (getline(input, line)) {
        sregex_iterator wrd_next(line.begin(), line.end(), wrd);
        //wrd_next++;
        smatch match = *wrd_next;
        string tn = match.str();
        wrd_next++;
        match = *wrd_next;
        int tl = stoi(match.str());
        tables[tn] = tl;
    }
    return 0;
}

int create_table(string s, int n) {
    mapT::iterator  it = tables.find(s);
    if (it == tables.end()) {
        tables[s] = n;
        ofstream file;
        file.open(s + ".dat", std::ofstream::out | std::ofstream::app);
        file.close();
        save_tables();
        return 0;
    }
    else
    {
        cout << "error of table creating" << endl;
        return 1;
    }
}

int delete_table(string s) {
    mapT::iterator  it = tables.find(s);
    if (it != tables.end()) {
        tables.erase(s);
        if (remove((s + ".dat").c_str()))
            cout << "Error removing file" << endl;
        save_tables();
        return 0;
    }
    else
    {
        cout << "error of table deleting" << endl;
        return 1;
    }
}

string str_toupper(string s) {
    transform(s.begin(), s.end(), s.begin(),
        [](unsigned char c) { return toupper(c); }
    );
    return s;
}




SClient::SClient(SOCKET s, SOCKADDR_IN sock_in)
{
    read_tables();

	c_sock = s;
	c_addr = sock_in;
	printf("Client created\n");
	//handle();
	HandleThread = CreateThread(NULL, 0, ThreadF, this, 0, &IdThread);

}


SClient::~SClient()
{
	TerminateThread(HandleThread, NULL);
	CloseHandle(HandleThread);
    save_tables();
}


int foo(string stroka)
{
    string stroke = str_toupper(stroka);
    string chislo = "(\\-*\\d+(.\\d+){0,1})";
    regex insrt("I\\s+\\w+\\s+\\(" + chislo + "(\\s*\\,\\s+" + chislo + ")*\\)");
    regex insrt2("\\(" + chislo + "(\\s*\\,\\s+" + chislo + ")*\\)");
    regex crt("C\\s+\\w+\\s+\\d+");
    regex dlt("D\\s+\\w+");
    regex chi(chislo);
    regex wrd("\\w+");
    try {
        sregex_iterator insrt_next(stroke.begin(), stroke.end(), insrt);
        sregex_iterator end;
        if (insrt_next != end) {
            sregex_iterator wrd_next(stroke.begin(), stroke.end(), wrd);
            wrd_next++;
            smatch match = *wrd_next;
            string table_name = match.str();
            sregex_iterator chisla_next(stroke.begin(), stroke.end(), insrt2);
            match = *chisla_next;
            string chisla = match.str();
            sregex_iterator next(chisla.begin(), chisla.end(), chi);
            int nn = 0;
            float chisla2[50];
            while (next != end) {
                nn += 1;
                smatch match = *next;
                chisla2[nn - 1] = stof(match.str());
                next++;
            }
            mapT::iterator  it = tables.find(table_name);
            if (it != tables.end()) {
                if (tables[table_name] == nn) {
                    time_t t = time(0);
                    ofstream file;
                    file.open(table_name + ".dat", ios::binary | ofstream::out | ofstream::in);
                    file.seekp(0, ios_base::end);
                    file.write((char*)(&t), sizeof(time_t));
                    file.write((char*)(&chisla2), sizeof(float) * nn);
                    file.close();
                }
                else {
                    cout << "error" << endl;
                    return 8;
                }
            }
            else {
                cout << "error" << endl;
                return 7;
            }
        }
        else {
            sregex_iterator crt_next(stroke.begin(), stroke.end(), crt);
            if (crt_next != end) {
                sregex_iterator wrd_next(stroke.begin(), stroke.end(), wrd);
                wrd_next++;
                smatch match = *wrd_next;
                string table_name = match.str();
                wrd_next++;
                match = *wrd_next;
                int nn = stoi(match.str());
                int err = create_table(table_name, nn);
                if (err > 0) {
                    //cout << "error" << endl;
                    return 6;
                }
            }
            else {
                sregex_iterator dlt_next(stroke.begin(), stroke.end(), dlt);
                if (dlt_next != end) {
                    sregex_iterator wrd_next(stroke.begin(), stroke.end(), wrd);
                    wrd_next++;
                    smatch match = *wrd_next;
                    string table_name = match.str();
                    int err = delete_table(table_name);
                    if (err > 0) {
                        cout << "error" << endl;
                        return 5;
                    }
                }
                else {
                    cout << "error" << endl;
                    return 4;
                }
            }
        }
    }
    catch (regex_error& e) {
        cout << "regex error" << endl;
        return 3;
    }
    return 0;
}


string str(char* a)
{
    int i;
    string s = "";
    for (i = 0; i< DEFAULT_BUFFER_LENGTH; i++) {
        s = s + a[i];
    }
    return s;
}


unsigned long SClient::CalculationThread()

{
    int er;
    char recvstr[DEFAULT_BUFFER_LENGTH];
    er = recv(c_sock, recvstr, DEFAULT_BUFFER_LENGTH, NULL);
    if (er > 0) {
        string stroka = str(recvstr);
        return foo(stroka);
    }
    else if (er == 0) {
        cout << "connection terminated\n";
        return 1;
    }
    else {
        cout<< "connection error"<<endl;
        return 2;
    }





    int k = recv(c_sock, buffer, sizeof(buffer), NULL);
    printf("%u\n", k);
    Sleep(30);
}

    /////////
