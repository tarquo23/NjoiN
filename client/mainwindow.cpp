#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QApplication>
#include <QDesktopWidget>
#include "loginwindow.h"
#include "accountinterface.h"
#include "inserttitle.h"
#include "simplecrypt.h"

socketManager *sock;
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)

{
    ui->setupUi(this);
    ui->label->setStyleSheet("background-image: url(:/images/Icons/logo-icon.png);background-repeat:none;background-position:center; text-align:top; color:yellow;");
    setWindowTitle("N Joi' N");

     model.setHorizontalHeaderItem( 0, new QStandardItem( "Documento" ) );
     model.setHorizontalHeaderItem( 1, new QStandardItem( "Creatore" ) );
     ui->treeView->setModel( &model );
     ui->treeView->setColumnWidth(0, 500);

     connect(ui->treeView, &QTreeView::clicked, this, &MainWindow::test);

}

MainWindow::~MainWindow()
{
    delete ui;
}
void MainWindow::newFile(){

    QList<QString> documentList;
    for(auto i = 0; i<this->model.rowCount(); i++) {
        auto index = this->model.index(i, 0);
        auto index1 = this->model.index(i, 1);
        auto stringa = this->model.data(index).toString() + "_" + this->model.data(index1).toString();
        documentList.push_back(stringa);
    }
    it = new InsertTitle(documentList);

    connect(it,&InsertTitle::setTitle,this,&MainWindow::receiveTitle);
    connect(it,&InsertTitle::showMw,this,&MainWindow::openMw);
    it->setUsername(this->username);

    it->exec();

}

int MainWindow::getSiteId() const
{
    return siteId;
}

void MainWindow::setSiteId(int value)
{
    siteId = value;
}

void MainWindow::setURI(QString u)
{
    openURI=u;
}

QString MainWindow::getURI()
{
    return openURI;
}

void MainWindow::sendNewImage(QByteArray &bArray){

    Message m;
    m.setAction('G');
    m.setSender(siteId);
    m.setParams({username, bArray});
    emit sendImage(&m);

}

void MainWindow::sendNewPwd(QString &oldPsw, QString &newPwd){
    Message m;
    m.setAction('P');
    m.setSender(siteId);
    m.setParams({username, oldPsw, newPwd});
    emit (sendPwd(&m));
}

void MainWindow::test(const QModelIndex &index)
{
   if(index.column()== 0) {

       QString creator = this->model.item(index.row(), 1)->text();
       QString documentName = index.data().toString();

       openURI=documentName + "_" + creator;

       Message m;
       m.setAction('R');
       m.setParams({openURI,username});
       m.setSender(siteId);

       flaglocal=1;
       emit(sendTextMessage(&m));
   }


}

void MainWindow::open_file_on_server(QListWidgetItem* s){
    Message m;
    m.setAction('R');
    m.setParams({s->text(),username});
    m.setSender(siteId);
    openURI=s->text();
    flaglocal=1;
    emit(sendTextMessage(&m));


}

void MainWindow::receivedFile(QList<Symbol> tmp){

    this->teWindow = new TextEditWindow();
    connect(teWindow,&TextEditWindow::openMW,this,&MainWindow::openMw);
    this->teWindow->setUri(openURI);
    this->teWindow->setWindowTitle(openURI.left(openURI.lastIndexOf('_'))+".txt");

    this->usersWindow = new QWidget();
    layout =new QHBoxLayout();
    layoutUsers = new QVBoxLayout();

    te = new TextEdit();
    te->setUsername(username);
    layout->addWidget(te);
    layout->addWidget(usersWindow);

    this->dockOnline = new QDockWidget(tr("Utenti online"));
    this->dockOnline->setParent(this->usersWindow);

    dockOnline->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    onlineUsers = new QListWidget(dockOnline);

    dockOnline->setWidget(onlineUsers);
    addDockWidget(Qt::RightDockWidgetArea, dockOnline);

    layoutUsers->addWidget(dockOnline);

    this->dockOffline = new QDockWidget(tr("Utenti offline"));
    this->dockOffline->setParent(this->usersWindow);

    dockOffline->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    offlineUsers = new QListWidget(dockOffline);

    dockOffline->setWidget(offlineUsers);
    addDockWidget(Qt::RightDockWidgetArea, dockOffline);

    layoutUsers->addWidget(dockOffline);

    emit(newTextEdit(te,siteId));
    te->setFileName(openURI.left(openURI.lastIndexOf('_')));
    te->setSiteid(siteId);
    te->setURI(openURI);
    connect(te,&TextEdit::openMW,this,&MainWindow::openMw);
    //cursore
    connect(this, &MainWindow::updateUsersOnTe, te, &TextEdit::updateUsersOnTe);




    layoutUsers->addStretch();
    layout->addLayout(layoutUsers);
    this->teWindow->setLayout(layout);
    this->teWindow->show();
    te->loadFile(tmp);

    auto flag = false;
    for(auto i = 0; i<this->model.rowCount(); i++) {
        auto index = this->model.index(i, 0);
        auto index1 = this->model.index(i, 1);
        auto stringa = this->model.data(index).toString() + "_" + this->model.data(index1).toString();
        if(openURI.compare(stringa)==0){
            flag = true;
            break;
        }
    }

    if(!flag)
        addElementforUser(openURI);

    this->hide();

}

void MainWindow::sendUri(Message m)
{
    m.setSender(siteId);
    SimpleCrypt crypto(Q_UINT64_C(0x0c2ad4a4acb9f023));
    QString decrypted = crypto.decryptToString(m.getParams()[0]);
    openURI=decrypted;
    m.setParams({decrypted});
    emit(sendTextMessage(&m));

}

void MainWindow::receiveURIerror()
{
    QMessageBox::critical(this,"ERRORE","URI non corretta");
}


void MainWindow::closeMw()
{
    on_actionClose_triggered();
}

void MainWindow::showUsers(Message m)
{
    this->onlineUsers->clear();
    this->offlineUsers->clear();
    this->onlineUserColor.clear();
    std::vector<QColor> listcolor={"#FF5252", "#B3FFC8", "#6190FF", "#FF80E6", "#FFE495",Qt::lightGray, "#B46767", "#9AD75D", "#686DA2", "#DC77F0", "#F0BC77"};

    bool online = true;

    QList<QString> onlineUserTE;

    for (auto user_siteId : m.getParams()) {

        if(user_siteId == "___")
            online = false;
        else {

            QStringList list = user_siteId.split("_");
            QString user = list.at(0);
            int siteId = list.at(1).toInt();

            QColor q;

            if(siteId == this->getSiteId())
                q = Qt::black;

            else {
                int pos=siteId%11;
                q=listcolor.at(pos);
            }


            if(online) {
                QList<QListWidgetItem*> a=this->onlineUsers->findItems(user,Qt::MatchExactly);
                if(a.size()==0){
                    this->onlineUsers->addItem(user);
                    this->onlineUsers->item(this->onlineUsers->count()-1)->setForeground(q);
                    onlineUserTE.append(user);
                    if(user!=username && !onlineUserColor.contains(user)){
                        onlineUserColor.insert(user,q);
                        emit(updateUsersOnTe(onlineUserColor));
                    }

                }
            }
            else{
                QList<QListWidgetItem*> a=this->offlineUsers->findItems(user,Qt::MatchExactly);
                if(a.size()==0){
                    this->offlineUsers->addItem(user);
                    this->offlineUsers->item(this->offlineUsers->count()-1)->setForeground(q);
                    onlineUserColor.remove(user);
                }
            }

        }

    }



}

void MainWindow::setImage(QPixmap im){
    image=im;
}

QPixmap MainWindow::getImage(){
    return image;
}
void MainWindow::setUsername(QString username){
    MainWindow::username=username;
}
QString MainWindow::getUsername(){
    return username;
}
QList<QString> MainWindow::getList(){
    return documents;
}
void MainWindow::setList(QList<QString> l){
    documents=l;
}

void MainWindow::receivedInfoAccount(Message& m){
    QString username = m.getParams().at(0);
    int siteId = m.getSender();

    QByteArray barray;
    barray = m.getParams().at(2).toLatin1(); //in base64

    QPixmap image;
    image.loadFromData(QByteArray::fromBase64(barray), "PNG");

    setUsername(username);
    setSiteId(siteId);
    setImage(image);

    QList<QString> tmp;
    long size = m.getParams().size();

    for(int i=3; i<size; i++){
        documents.append(m.getParams().at(i));
        addElementforUser(m.getParams().at(i));
    }
}

void MainWindow::on_pushButton_clicked()
{
    newFile();

}

void MainWindow::addElementforUser(QString string){   

    auto strings = string.split('_');
    QString creator;
    QString documentName;

    if(strings.size()==2) {
        creator = strings[1];
        documentName = strings[0];
    }
    else {      //nel titolo c'è un underscore
        creator = strings[strings.size()-1];
        documentName = string.left(string.lastIndexOf('_'));
    }


    QStandardItem *item1 = new QStandardItem(documentName);
    QStandardItem *item2 = new QStandardItem(creator);

    this->model.appendRow({item1, item2});

}

void MainWindow::on_actionNew_triggered()
{
    newFile();
}

void MainWindow::on_actionAccount_triggered()
{
    AccountInterface ai;
    ai.setUsername(username);
    ai.setImagePic(image);
    connect(&ai, &AccountInterface::changeImage, this, &MainWindow::sendNewImage);
    connect(this, &MainWindow::receivedNewImage, &ai, &AccountInterface::receiveNewImage);
    connect(&ai, &AccountInterface::changePassword, this, &MainWindow::sendNewPwd);
    connect(this, &MainWindow::receivedNewPsw, &ai, &AccountInterface::receiveNewPsw);
    ai.exec();

}

void MainWindow::receiveNewImageMW(Message &m){
    emit(receivedNewImage(m));
    return;
}
void MainWindow::receiveNewPswMW(Message &m){
    emit(receivedNewPsw(m));
}

void MainWindow::on_actionClose_triggered()
{
    this->close();
}



void MainWindow::on_pushButton_2_clicked()
{   i = new Inserturi();
    connect(i,&Inserturi::sendUri,this,&MainWindow::sendUri);
    i->exec();
}


void MainWindow::receiveTitle(QString title)
{
    this->hide();

    this->teWindow = new TextEditWindow();
    connect(teWindow,&TextEditWindow::openMW,this,&MainWindow::openMw);
    this->teWindow->setUri(title+"_"+username);
    this->teWindow->setWindowTitle(title+".txt");

    this->usersWindow = new QWidget();
    layout =new QHBoxLayout();
    layoutUsers = new QVBoxLayout();

    te = new TextEdit();    
    te->setUsername(username);
    layout->addWidget(te);
    layout->addWidget(usersWindow);
    this->dockOnline = new QDockWidget(tr("Utenti online"));
    this->dockOnline->setParent(this->usersWindow);

    dockOnline->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    onlineUsers = new QListWidget(dockOnline);
    onlineUsers->addItem(this->username);

    dockOnline->setWidget(onlineUsers);
    addDockWidget(Qt::RightDockWidgetArea, dockOnline);

    layoutUsers->addWidget(dockOnline);

    this->dockOffline = new QDockWidget(tr("Utenti offline"));
    this->dockOffline->setParent(this->usersWindow);

    dockOffline->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    offlineUsers = new QListWidget(dockOffline);

    dockOffline->setWidget(offlineUsers);
    addDockWidget(Qt::RightDockWidgetArea, dockOffline);

    layoutUsers->addWidget(dockOffline);


    const QRect availableGeometry = QApplication::desktop()->availableGeometry(te);
    te->resize(availableGeometry.width() / 2, (availableGeometry.height() * 2) / 3);
    te->move((availableGeometry.width() - te->width()) / 2,
           (availableGeometry.height() - te->height()) / 2);
    te->setFileName(title);
    te->setURI(title+"_"+username);
    openURI = te->getURI();
    te->setSiteid(siteId);

    auto flag = false;
    for(auto i = 0; i<this->model.rowCount(); i++) {
        auto index = this->model.index(i, 0);
        auto index1 = this->model.index(i, 1);
        auto stringa = this->model.data(index).toString() + "_" + this->model.data(index1).toString();
        if(openURI.compare(stringa)==0){
            flag = true;
            break;
        }
    }

    if(!flag)
        addElementforUser(openURI);

    te->fileNew();
    connect(te,&TextEdit::openMW,this,&MainWindow::openMw);
//cursore
   connect(this, &MainWindow::updateUsersOnTe, te, &TextEdit::updateUsersOnTe);


    Message m;
    m.setAction('C');
    m.setParams({title, this->getUsername()});
    m.setSender(siteId);
    emit(sendTextMessage(&m));
    emit(newTextEdit(te,siteId));


    layoutUsers->addStretch();
    layout->addLayout(layoutUsers);
    this->teWindow->setLayout(layout);
    this->teWindow->show();
}

void MainWindow::openMw(QString fileName)
{
    this->show();

    if (fileName!="") {
        Message m,mc;
        mc.setAction('Z');
        mc.setSender(this->getSiteId());
        mc.setParams({QString::number(-24),username});
        emit(sendTextMessage(&mc));

        m.setAction('X');
        m.setSender(this->getSiteId());
        m.setParams({fileName, this->username});
        emit(sendTextMessage(&m));
        //messaggio per cursore

        this->openURI.clear();

        emit(closeTextEdit(this->te));
    }
}

void MainWindow::documentClosed(QString fileName)
{

}

void MainWindow::on_actionOpen_triggered()
{
    i = new Inserturi();
    connect(i,&Inserturi::sendUri,this,&MainWindow::sendUri);
    i->exec();
}
