#ifndef SOCKETMANAGER_H
#define SOCKETMANAGER_H

#include <QObject>
#include "QtWebSockets/qwebsocketserver.h"
#include "QtWebSockets/qwebsocket.h"

#include "message.h"

class Message;

class SocketManager : public QObject
{
    Q_OBJECT

public:
    explicit SocketManager(QObject *parent = nullptr);
    void sendError (std::string); //da fare con le classi apposite per gli errori

    QMap<int, QWebSocket *> getClients() const;
    void setClients(const QMap<int, QWebSocket *> &value);

signals:
    void newMessage(Message* m);

public slots:
    void messageToUser (Message* m, int siteId);
    void fileToUser (std::vector<Symbol> file, int user);
    void onNewConnection();
    void processTextMessage(QString message);
    void socketDisconnected();

private:
    QWebSocketServer* qWebSocketServer;
    QMap<int, QWebSocket *> clients;

};

#endif // SOCKETMANAGER_H
