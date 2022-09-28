#include <pthread.h>
#include <semaphore.h>

#include <unistd.h>
#include <iostream>
#include <stdio.h>
#include <string>
#include <vector>

using namespace std;


struct Brief {long empfaengerid;
              string text;
              };
struct Zusteller {pthread_t zustellerThreadID;
                  pthread_mutex_t* zustellerMutex;
                  sem_t* zentrumWarten;
                  sem_t* klingeln;
                  Brief* briefBeutel[15];
                  };
struct Empfaenger {pthread_t empfaengerThreadID;
                   pthread_mutex_t* empfaengerMutex;
                   sem_t* klingeln;
                   vector<Brief*> briefkasten;
                   };

pthread_mutex_t* mutexTemp;
sem_t* semaphoreTemp;
Brief* briefTemp;

pthread_t zentrum;
vector<Zusteller> zustellerListe;
vector<Empfaenger> empfaengerListe;
sem_t offenePositionen;
sem_t vorlesen;

bool logout = false;
int interval = 0;
int nextEmpfaenger = 0;

int findeMeinePosition(long myID, bool zustellerOderEmpfaenger) {
    if(zustellerOderEmpfaenger) {
        for(int i = 0; i < zustellerListe.size(); i++) {
            if(zustellerListe[i].zustellerThreadID == myID) {
                return i;
            }
        }
    } else {
        for(int i = 0; i < empfaengerListe.size(); i++) {
            if(empfaengerListe[i].empfaengerThreadID == myID) {
                return i;
            }
        }
    }
    return 0;
}

void briefeAuffuellen(Zusteller* zusteller) {
    for(int i = 0; i < 15; i++) {
        usleep(interval);
        briefTemp = new Brief;
        briefTemp->empfaengerid = empfaengerListe[nextEmpfaenger].empfaengerThreadID;
        briefTemp->text = "hello world";
        zusteller->briefBeutel[i] = briefTemp;
        nextEmpfaenger++;
        if(nextEmpfaenger >= empfaengerListe.size()) {
            nextEmpfaenger = 0;
        }
    }
}

void *ZentrumThread(void *tid) {
    usleep(1000000);
    while(!logout) {
        for(auto &zusteller : zustellerListe) {
            sem_wait(&offenePositionen);
            if(!pthread_mutex_trylock(zusteller.zustellerMutex)) {
                cout << "zentrum fuellt briefe auf" << endl;
                briefeAuffuellen(&zusteller);
                pthread_mutex_unlock(zusteller.zustellerMutex);
                sem_post(zusteller.zentrumWarten);
            }
        }
    }
    pthread_exit(NULL);
}

int getNextEmpfaengerPosition(long empfaengerid) {
    for(int i = 0; i < empfaengerListe.size(); i++) {
        if(empfaengerListe[i].empfaengerThreadID == empfaengerid) {
            return i;
        }
    }
    return 0;
}

void briefeAusteilen(int zustellerPosition) {
    for(int i = 0; i < 15; i++) {
        int nextEmpfaengerPosition = getNextEmpfaengerPosition(zustellerListe[zustellerPosition].briefBeutel[i]->empfaengerid);
        usleep(500000);
        pthread_mutex_lock(empfaengerListe[nextEmpfaengerPosition].empfaengerMutex);
        sem_post(empfaengerListe[nextEmpfaengerPosition].klingeln);
        cout << zustellerListe[zustellerPosition].zustellerThreadID << " ich habe geklingelt" << endl;
        usleep(500000);
        sem_wait(empfaengerListe[nextEmpfaengerPosition].klingeln);
        empfaengerListe[nextEmpfaengerPosition].briefkasten.push_back(zustellerListe[zustellerPosition].briefBeutel[i]);
        cout << "briefe eingewofen" << endl;
        pthread_mutex_unlock(empfaengerListe[nextEmpfaengerPosition].empfaengerMutex);
        zustellerListe[zustellerPosition].briefBeutel[i] = NULL;
    }
    usleep(2000000);
}

void *zustellerThread(void *tid) {
    cout << "gestartet" << endl;
    usleep(1000000);
    long myID = pthread_self();
    int myPosition = findeMeinePosition(myID, true);
    while(!logout) {
        cout << myID << " wartet auf zentrum" << endl;
        sem_wait(zustellerListe[myPosition].zentrumWarten);
        cout << myID << " beginnt mit austeilen" << endl;
        pthread_mutex_lock(zustellerListe[myPosition].zustellerMutex);
        briefeAusteilen(myPosition);
        cout << myID << " ist fertig mit austeilen" << endl;
        pthread_mutex_unlock(zustellerListe[myPosition].zustellerMutex);
        sem_post(&offenePositionen);
    }
    pthread_exit(NULL);
}

int briefVorlesen(long vorleserID, Brief* brief, int briefCounter) {
    cout << "ich bin empfaenger " << vorleserID << " ich habe einen brief fuer " << brief->empfaengerid << " der text lautet " << brief->text;
    cout << " --- dies ist mein " << briefCounter << ". brief" << endl;
    briefCounter++;
    return briefCounter;
}

void *empfaengerThread(void *tid) {
    usleep(1000000);
    int briefCounter = 1;
    long myID = pthread_self();
    int myPosition = findeMeinePosition(myID, false);
    while(!logout) {
        sem_wait(empfaengerListe[myPosition].klingeln);
        sem_post(empfaengerListe[myPosition].klingeln);
        cout << myID << " habe die tür auf gemacht" << endl;
        pthread_mutex_lock(empfaengerListe[myPosition].empfaengerMutex);
        sem_wait(&vorlesen);
        for(int i = empfaengerListe[myPosition].briefkasten.size()-1; i >= 0; i--) {
            briefCounter = briefVorlesen(myID, empfaengerListe[myPosition].briefkasten[i], briefCounter);
            empfaengerListe[myPosition].briefkasten.pop_back();
        }
        sem_post(&vorlesen);
        pthread_mutex_unlock(empfaengerListe[myPosition].empfaengerMutex);
    }
    pthread_exit(NULL);
}

void initNewZusteller() {
    struct Zusteller zustellerTemp;
    pthread_t zustellerIDTmp;
    mutexTemp = new pthread_mutex_t();
    semaphoreTemp = new sem_t();
    sem_init(semaphoreTemp, 0, 0);
    zustellerTemp.zentrumWarten = semaphoreTemp;
    semaphoreTemp = new sem_t();
    sem_init(semaphoreTemp, 0, 0);
    zustellerTemp.klingeln = semaphoreTemp;
    pthread_create(&zustellerIDTmp, NULL, zustellerThread, (void*) NULL);
    pthread_mutex_init(mutexTemp, NULL);
    zustellerTemp.zustellerMutex = mutexTemp;
    zustellerTemp.zustellerThreadID = zustellerIDTmp;
    zustellerListe.push_back(zustellerTemp);
}

void initNewEmpfaenger() {
    struct Empfaenger empfaengerTemp;
    pthread_t empfaengerIDTemp;
    mutexTemp = new pthread_mutex_t();
    semaphoreTemp = new sem_t();
    sem_init(semaphoreTemp, 0, 0);
    empfaengerTemp.klingeln = semaphoreTemp;
    pthread_create(&empfaengerIDTemp, NULL, empfaengerThread, (void*) NULL);
    pthread_mutex_init(mutexTemp, NULL);
    empfaengerTemp.empfaengerMutex = mutexTemp;
    empfaengerTemp.empfaengerThreadID = empfaengerIDTemp;
    empfaengerListe.push_back(empfaengerTemp);
}

void init(int anzahlZusteller, int anzahlEmpfaenger) {
    sem_init(&offenePositionen, 0, anzahlZusteller);
    sem_init(&vorlesen, 0, 1);
    pthread_create(&zentrum, NULL, ZentrumThread, (void*) NULL);
    for(int i = 0; i < anzahlZusteller; i++) {
        initNewZusteller();
    }
    for(int i = 0; i < anzahlEmpfaenger; i++) {
        initNewEmpfaenger();
    }
}

void waitForJoin(int anzahlZusteller, int anzahlEmpfaenger) {
    pthread_join(zentrum, NULL);
    for(int i = 0; i < anzahlZusteller; i++) {
        pthread_join(zustellerListe[i].zustellerThreadID, NULL);
    }
    for(int i = 0; i < anzahlEmpfaenger; i++) {
        pthread_join(empfaengerListe[i].empfaengerThreadID, NULL);
    }
}

int main() {
    int anzahlZusteller, anzahlEmpfaenger = 0;
    cout << "wie viele zusteller soll es geben? ";
    cin >> anzahlZusteller;
    cout << "wie viele empfaenger soll es geben? ";
    cin >> anzahlEmpfaenger;
    cout << "wie oft sollen briefe erzeugt werden? (angabe in mikrosekunden)";
    cin >> interval;
    init(anzahlZusteller, anzahlEmpfaenger);
    while(!logout) {
        string logoutstring;
        cin >> logoutstring;
        if(logoutstring == "l") {
            logout = true;
        }
    }
    waitForJoin(anzahlZusteller, anzahlEmpfaenger);
    return 0;
}
