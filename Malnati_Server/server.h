#ifndef SERVER_H
#define SERVER_H

#include <QObject>
#include <map>
#include <queue>
#include "symbol.h"
#include "sharedDocument.h"
#include "socketManager.h"
#include "databaseManager.h"
#include "account.h"
#include "message.h"


class SharedDocument;
//class Account;
class Message;

class Server: public QObject
{
Q_OBJECT

private:
    //std::map<std::string, std::vector<Symbol>> documents;     in questo caso il file andrebbe aggiornato anche lato server (ottimizzazione futura = OF)
    //std::map<int, Account> onlineAccounts;
    //std::queue<Message> codaMessaggi;             Questo sarebbe il modo classico di gestirlo in c++, provare a usare invece signal e slots per gestire gli eventi


public:
    SocketManager* socketMan;
    DatabaseManager* dbMan;

    explicit Server(QObject *parent = nullptr);
    void dispatchMessage(Message* mes);          //capisci a quali client inviare il messaggi


signals:

    void openFileFromDB (std::string nome);                      // chiedi al DB manager di recuperare il file dal DB
    void closed();  //TODO: decidere quando emettere questo segnale

public slots:
    void processMessage(Message* mes);
    //void tryInLocal (std::string);              //controlla se il file è tra quelli nella memoria locale del server (in documents) OF

};

#endif // SERVER_H