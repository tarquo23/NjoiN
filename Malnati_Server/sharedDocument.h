#ifndef SHAREDDOCUMENT_H
#define SHAREDDOCUMENT_H

#include <QString>
#include <QList>
#include "symbol.h"


class SharedDocument
{
    QString name;
    QList<int> userAllowed;
    int creator;
    bool open;
    QList<Symbol> symbols;

public:
    SharedDocument();
    SharedDocument(QString name, int creator);
    QString getName() const;
    void setName(const QString &value);
    QList<int> getUserAllowed() const;
    void setUserAllowed(const QList<int> &value);
    int getCreator() const;
    bool getOpen() const;
    void setOpen(bool value);
    QList<Symbol> getSymbols() const;
    void setSymbols(const QList<Symbol> &value);
};

#endif // SHAREDDOCUMENT_H
