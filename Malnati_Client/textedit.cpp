#include <QColor>
#include <QAction>
#include <QApplication>
#include <QClipboard>
#include <QColorDialog>
#include <QComboBox>
#include <QFontComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QMenu>
#include <QMenuBar>
#include <QTextCodec>
#include <QTextEdit>
#include <QStatusBar>
#include <QToolBar>
#include <QTextCursor>
#include <QTextDocumentWriter>
#include<QPushButton>
#include <QTextList>
#include <QtDebug>
#include <QCloseEvent>
#include <QMessageBox>
#include <QMimeData>
#if defined(QT_PRINTSUPPORT_LIB)
#include <QtPrintSupport/qtprintsupportglobal.h>
#if QT_CONFIG(printer)
#if QT_CONFIG(printdialog)
#include <QPrintDialog>
#endif
#include <QPrinter>
#if QT_CONFIG(printpreviewdialog)
#include <QPrintPreviewDialog>
#endif
#endif
#endif
#include "textedit.h"
#include<QDebug>
#include "message.h"
#include "showuridialog.h"

#ifdef Q_OS_MAC
const QString rsrcPath = ":/images/mac";
#else
const QString rsrcPath = ":/images/win";
#endif




TextEdit::TextEdit(QWidget *parent)
    : QMainWindow(parent)
{
#ifdef Q_OS_OSX
    setUnifiedTitleAndToolBarOnMac(true);
#endif
    setWindowTitle(QCoreApplication::applicationName());





    textEdit = new QTextEdit(this);
//    textEdit->textCursor().setPosition(textEdit->document()->end().position());
//    textEdit->textCursor().setPosition(40960);
//    textEdit->textCursor().setPosition(0);
    textEdit->selectAll();
    externAction=false;

    connect(textEdit, &QTextEdit::currentCharFormatChanged,
            this, &TextEdit::currentCharFormatChanged);
    connect(textEdit, &QTextEdit::cursorPositionChanged,
            this, &TextEdit::cursorPositionChanged);


    setCentralWidget(textEdit);

    setToolButtonStyle(Qt::ToolButtonFollowStyle);
    setupFileActions();
    setupEditActions();
    setupTextActions();
    setupUriActions();
    setupBackgroundAction();

    QFont textFont("Helvetica");
    textEdit->setFont(textFont);
    fontChanged(textEdit->font());
    alignmentChanged(textEdit->alignment());
    textEdit->setFontFamily("kalapi");
    localFamily="kalapi";
    textEdit->setFontPointSize(12);
    localsize=12;
    textEdit->currentCharFormat().setFontItalic(false);
    textEdit->currentCharFormat().setFontUnderline(false);
    this->localOperation = false;


    connect(textEdit->document(), &QTextDocument::modificationChanged,
            this, &QWidget::setWindowModified);
    connect(textEdit->document(), &QTextDocument::undoAvailable,
            actionUndo, &QAction::setEnabled);
    connect(textEdit->document(), &QTextDocument::redoAvailable,
            actionRedo, &QAction::setEnabled);
    connect(textEdit->document(), &QTextDocument::contentsChange,
            this, &TextEdit::onTextChanged);


    connect(textEdit, &QTextEdit::cursorPositionChanged, this, [&](){
        int pos = textEdit->textCursor().position();
        if (!localOperation || handlingOperation )
            localOperation = false;
        else{
            Message m;
            m.setAction('Z');
            m.setSender(siteid);
            m.setParams({QString::number(pos),username});
            emit sendTextMessage(&m);
        }
 });



    setWindowModified(textEdit->document()->isModified());
    actionUndo->setEnabled(textEdit->document()->isUndoAvailable());
    actionRedo->setEnabled(textEdit->document()->isRedoAvailable());
    textEdit->textChanged();
#ifndef QT_NO_CLIPBOARD
    actionCut->setEnabled(false);
    connect(textEdit, &QTextEdit::copyAvailable, actionCut, &QAction::setEnabled);
    actionCopy->setEnabled(false);
    connect(textEdit, &QTextEdit::copyAvailable, actionCopy, &QAction::setEnabled);

    connect(QApplication::clipboard(), &QClipboard::dataChanged, this, &TextEdit::clipboardDataChanged);
#endif

    textEdit->setFocus();
    setCurrentFileName();

#ifdef Q_OS_MACOS
    // Use dark text on light background on macOS, also in dark mode.
    QPalette pal = textEdit->palette();
    pal.setColor(QPalette::Base, QColor(Qt::white));
    pal.setColor(QPalette::Text, QColor(Qt::black));
    textEdit->setPalette(pal);
#endif


}



void TextEdit::closeEvent(QCloseEvent */* unused */)
{
    emit(openMW(this->getURI()));

}

void TextEdit::setupFileActions()
{
    QToolBar *tb = addToolBar(tr("File Actions"));
    QMenu *menu = menuBar()->addMenu(tr("&File"));

#ifndef QT_NO_PRINTER
    const QIcon printIcon = QIcon::fromTheme("document-print", QIcon(rsrcPath + "/fileprint.png"));
    QAction *a = menu->addAction(printIcon, tr("&Print..."), this, &TextEdit::filePrint);
    a->setPriority(QAction::LowPriority);
    a->setShortcut(QKeySequence::Print);
    tb->addAction(a);

    const QIcon filePrintIcon = QIcon::fromTheme("fileprint", QIcon(rsrcPath + "/fileprint.png"));
    menu->addAction(filePrintIcon, tr("Print Preview..."), this, &TextEdit::filePrintPreview);

    const QIcon exportPdfIcon = QIcon::fromTheme("exportpdf", QIcon(rsrcPath + "/exportpdf.png"));
    a = menu->addAction(exportPdfIcon, tr("&Export PDF..."), this, &TextEdit::filePrintPdf);
    a->setPriority(QAction::LowPriority);
    a->setShortcut(Qt::CTRL + Qt::Key_D);
    tb->addAction(a);

#endif
}

void TextEdit::setupEditActions()
{
    QToolBar *tb = addToolBar(tr("Edit Actions"));
    QMenu *menu = menuBar()->addMenu(tr("&Edit"));

    const QIcon undoIcon = QIcon::fromTheme("edit-undo", QIcon(rsrcPath + "/editundo.png"));
    actionUndo = menu->addAction(undoIcon, tr("&Undo"), textEdit, &QTextEdit::undo);
    actionUndo->setPriority(QAction::LowPriority);
    actionUndo->setShortcut(QKeySequence::Undo);
    tb->addAction(actionUndo);

    const QIcon redoIcon = QIcon::fromTheme("edit-redo", QIcon(rsrcPath + "/editredo.png"));
    actionRedo = menu->addAction(redoIcon, tr("&Redo"), textEdit, &QTextEdit::redo);
    actionRedo->setPriority(QAction::LowPriority);
    actionRedo->setShortcut(QKeySequence::Redo);
    tb->addAction(actionRedo);
    menu->addSeparator();

#ifndef QT_NO_CLIPBOARD
    const QIcon cutIcon = QIcon::fromTheme("edit-cut", QIcon(rsrcPath + "/editcut.png"));
    actionCut = menu->addAction(cutIcon, tr("Cu&t"), textEdit, &QTextEdit::cut);
    actionCut->setPriority(QAction::LowPriority);
    actionCut->setShortcut(QKeySequence::Cut);
    tb->addAction(actionCut);

    const QIcon copyIcon = QIcon::fromTheme("edit-copy", QIcon(rsrcPath + "/editcopy.png"));
    actionCopy = menu->addAction(copyIcon, tr("&Copy"), textEdit, &QTextEdit::copy);
    actionCopy->setPriority(QAction::LowPriority);
    actionCopy->setShortcut(QKeySequence::Copy);
    tb->addAction(actionCopy);

    const QIcon pasteIcon = QIcon::fromTheme("edit-paste", QIcon(rsrcPath + "/editpaste.png"));
    actionPaste = menu->addAction(pasteIcon, tr("&Paste"), textEdit, &QTextEdit::paste);
    actionPaste->setPriority(QAction::LowPriority);
    actionPaste->setShortcut(QKeySequence::Paste);
    tb->addAction(actionPaste);
    if (const QMimeData *md = QApplication::clipboard()->mimeData())
        actionPaste->setEnabled(md->hasText());
#endif
}

void TextEdit::setupTextActions()
{
    QToolBar *tb = addToolBar(tr("Format Actions"));
    QMenu *menu = menuBar()->addMenu(tr("F&ormat"));

    const QIcon boldIcon = QIcon::fromTheme("format-text-bold", QIcon(rsrcPath + "/textbold.png"));
    actionTextBold = menu->addAction(boldIcon, tr("&Bold"), this, &TextEdit::textBold);
    actionTextBold->setShortcut(Qt::CTRL + Qt::Key_B);
    actionTextBold->setPriority(QAction::LowPriority);
    QFont bold;
    bold.setBold(true);
    actionTextBold->setFont(bold);
    tb->addAction(actionTextBold);
    actionTextBold->setCheckable(true);

    const QIcon italicIcon = QIcon::fromTheme("format-text-italic", QIcon(rsrcPath + "/textitalic.png"));
    actionTextItalic = menu->addAction(italicIcon, tr("&Italic"), this, &TextEdit::textItalic);
    actionTextItalic->setPriority(QAction::LowPriority);
    actionTextItalic->setShortcut(Qt::CTRL + Qt::Key_I);
    QFont italic;
    italic.setItalic(true);
    actionTextItalic->setFont(italic);
    tb->addAction(actionTextItalic);
    actionTextItalic->setCheckable(true);

    const QIcon underlineIcon = QIcon::fromTheme("format-text-underline", QIcon(rsrcPath + "/textunder.png"));
    actionTextUnderline = menu->addAction(underlineIcon, tr("&Underline"), this, &TextEdit::textUnderline);
    actionTextUnderline->setShortcut(Qt::CTRL + Qt::Key_U);
    actionTextUnderline->setPriority(QAction::LowPriority);
    QFont underline;
    underline.setUnderline(true);
    actionTextUnderline->setFont(underline);
    tb->addAction(actionTextUnderline);
    actionTextUnderline->setCheckable(true);

    menu->addSeparator();

    const QIcon leftIcon = QIcon::fromTheme("format-justify-left", QIcon(rsrcPath + "/textleft.png"));
    actionAlignLeft = new QAction(leftIcon, tr("&Left"), this);
    actionAlignLeft->setShortcut(Qt::CTRL + Qt::Key_L);
    actionAlignLeft->setCheckable(true);
    actionAlignLeft->setPriority(QAction::LowPriority);
    const QIcon centerIcon = QIcon::fromTheme("format-justify-center", QIcon(rsrcPath + "/textcenter.png"));
    actionAlignCenter = new QAction(centerIcon, tr("C&enter"), this);
    actionAlignCenter->setShortcut(Qt::CTRL + Qt::Key_E);
    actionAlignCenter->setCheckable(true);
    actionAlignCenter->setPriority(QAction::LowPriority);
    const QIcon rightIcon = QIcon::fromTheme("format-justify-right", QIcon(rsrcPath + "/textright.png"));
    actionAlignRight = new QAction(rightIcon, tr("&Right"), this);
    actionAlignRight->setShortcut(Qt::CTRL + Qt::Key_R);
    actionAlignRight->setCheckable(true);
    actionAlignRight->setPriority(QAction::LowPriority);
    const QIcon fillIcon = QIcon::fromTheme("format-justify-fill", QIcon(rsrcPath + "/textjustify.png"));
    actionAlignJustify = new QAction(fillIcon, tr("&Justify"), this);
    actionAlignJustify->setShortcut(Qt::CTRL + Qt::Key_J);
    actionAlignJustify->setCheckable(true);
    actionAlignJustify->setPriority(QAction::LowPriority);

    // Make sure the alignLeft  is always left of the alignRight
    QActionGroup *alignGroup = new QActionGroup(this);
    connect(alignGroup, &QActionGroup::triggered, this, &TextEdit::textAlign);

    if (QApplication::isLeftToRight()) {
        alignGroup->addAction(actionAlignLeft);
        alignGroup->addAction(actionAlignCenter);
        alignGroup->addAction(actionAlignRight);
    } else {
        alignGroup->addAction(actionAlignRight);
        alignGroup->addAction(actionAlignCenter);
        alignGroup->addAction(actionAlignLeft);
    }
    alignGroup->addAction(actionAlignJustify);

    tb->addActions(alignGroup->actions());
    menu->addActions(alignGroup->actions());


    tb = addToolBar(tr("Format Actions"));
    tb->setAllowedAreas(Qt::TopToolBarArea | Qt::BottomToolBarArea);
    addToolBarBreak(Qt::TopToolBarArea);
    addToolBar(tb);

    comboFont = new QFontComboBox(tb);
    tb->addWidget(comboFont);
    connect(comboFont, QOverload<const QString &>::of(&QComboBox::activated), this, &TextEdit::textFamily);

    comboSize = new QComboBox(tb);
    comboSize->setObjectName("comboSize");
    tb->addWidget(comboSize);
    comboSize->setEditable(true);

    const QList<int> standardSizes = QFontDatabase::standardSizes();
    foreach (int size, standardSizes)
        comboSize->addItem(QString::number(size));
    comboSize->setCurrentIndex(standardSizes.indexOf(QApplication::font().pointSize()));

    connect(comboSize, QOverload<const QString &>::of(&QComboBox::activated), this, &TextEdit::textSize);
}

void TextEdit::setupUriActions()
{
    QToolBar *tb = addToolBar(tr("Mostra Uri"));
    QMenu *menu = menuBar()->addMenu(tr("&Uri"));

    const QIcon newIcon = QIcon::fromTheme("showUri", QIcon(rsrcPath + "/../Icons/icons8-collegamento-24.png"));
    QAction *a = menu->addAction(newIcon, tr("&showUri"), this, &TextEdit::showUriWindow);
    tb->addAction(a);
    a->setPriority(QAction::LowPriority);

}

void TextEdit::setupBackgroundAction()
{
    QToolBar *tb = addToolBar(tr("Modifica Background"));
    QMenu *menu = menuBar()->addMenu(tr("&Background"));

    const QIcon newIcon = QIcon::fromTheme("ModifyBackground", QIcon(rsrcPath + "/../Icons/icons2.png"));
    QAction *a = menu->addAction(newIcon, tr("&ModifyBackground"), this, &TextEdit::modifyBackground);
    tb->addAction(a);
    a->setPriority(QAction::LowPriority);
}



void TextEdit::setCrdt(Crdt *crdtclient)
{
    crdt=crdtclient;
}

void TextEdit::setSocketM(socketManager *sockclient)
{
    sockm=sockclient;
    connect(this, &TextEdit::sendMessage, sockm, &socketManager::binaryMessageToServer);
    connect(this, &TextEdit::sendTextMessage, sockm, &socketManager::messageToServer);
    connect(sockm,&socketManager::receiveAllign,this,&TextEdit::receiveAllign);
    connect(sockm, &socketManager::updateCursor,this,&TextEdit::moveCursor);

}

void TextEdit::setFileName(QString fileName)
{
    this->fileName = fileName;
}

void TextEdit::setURI(QString u)
{
    URI=u;
}

QString TextEdit::getURI()
{
    return URI;
}


void TextEdit::setCurrentFileName()
{
    QString shownName;
    if (this->fileName.isEmpty())
        shownName = "untitled.txt";
    else
        shownName = QFileInfo(this->fileName).fileName();

    setWindowTitle(tr("%1[*] - %2").arg(shownName, QCoreApplication::applicationName()));
    setWindowModified(false);

}

void TextEdit::setUsername(QString usern)
{
    username=usern;
}

void TextEdit::fileNew()
{
    setCurrentFileName();

}

void TextEdit::receiveSymbol(Message *m)
{
    handlingOperation = true;
    externAction=true;
    localOperation = false;
    QTextCursor curs = textEdit->textCursor(), oldcurs;
    QColor col;
    QTextCharFormat qform, preqform;
    if(colors.find(m->getSymbol().getSiteId())==colors.end()){
        int pos=m->getSymbol().getSiteId()%11;
        QColor q=listcolor.at(pos);
        colors.insert(m->getSymbol().getSiteId(),q);
    }
    if(backgroundOp==true){
        col=colors.take(m->getSymbol().getSiteId());
        qform.setBackground(col);
    }
    else
        qform.setBackground(Qt::transparent);
    qform.setFontFamily(m->getSymbol().getFamily());
    qform.setFontItalic(m->getSymbol().getItalic());
    qform.setFontUnderline(m->getSymbol().getUnderln());
    qform.setFontPointSize(m->getSymbol().getSize());
    if(m->getSymbol().getBold())
        qform.setFontWeight(QFont::Bold);

    int position,oldposition;
    oldposition=textEdit->textCursor().position();
    Symbol tmp = m->getSymbol();
    if(m->getAction()=='I'){
        position=crdt->remoteinsert(tmp);
        if(position == -1) return;
        curs.setPosition(position);
        if(tmp.getValue()=='\0')
            curs.insertText((QChar)'\n',qform);
        else
            curs.insertText((QChar)tmp.getValue(),qform);
    }
    else if(m->getAction()=='D'){
        position=crdt->remotedelete(tmp);
        curs.setPosition(position+1);
        curs.deletePreviousChar();

    }

    updateCursors();
    handlingOperation = false;


}

void TextEdit::receiveAllign(Message m)
{
    QTextCursor c=textEdit->textCursor();
    int start,end;
    QChar a;
    Qt::Alignment al;
    start=m.getParams().at(0).toInt();
    end=m.getParams().at(1).toInt();
    a=m.getParams().at(2).toLatin1().at(0);
    al=insertalign(a);
    for(int i=start;i<=end;i++){
        crdt->getSymbols()[i].setAlign(a);
    }
    c.setPosition(start);
    textEdit->setTextCursor(c);
    alignAction=false;
    externAction=true;
    textEdit->setAlignment(al);

}

void TextEdit::setSiteid(int s)
{
    siteid=s;

}

void TextEdit::updateUsersOnTe(QMap<QString,QColor> users)
{

    for(auto userKey : users.keys()){
    QFont font("American Typewriter", 10, QFont::Bold);
    QLabel *remoteLabel = new QLabel(this);
    QColor color(users.value(userKey));
    remoteLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
    remoteLabel->setStyleSheet("color:"+color.name()+";background-color:transparent;border: 1px solid transparent;border-left-color:"+color.name()+";");
    remoteLabel->setFont(font);
//    User newUser = { userKey, remoteLabel, QTextCursor(textEdit->textCursor())};
    User newUser = { userKey, remoteLabel, QTextCursor(textEdit->document())};
    m_onlineUsers[userKey] = newUser;
    // 2. Draw the remote cursor at position 0
    QTextCursor& remoteCursor = m_onlineUsers[userKey].cursor;
    remoteCursor=textEdit->textCursor();
    QRect curCoord = textEdit->cursorRect();
    int height = curCoord.bottom()-curCoord.top();
    remoteLabel->resize(1000, height+5);

    /* update label dimension according to remote cursor position */
    QFont l_font=remoteLabel->font();
    QTextCharFormat fmt=remoteCursor.charFormat();
    int font_size=static_cast<int>(fmt.fontPointSize());
    if(font_size==0)
        font_size=DEFAULT_SIZE;
    QFont new_font(l_font.family(),static_cast<int>((font_size/2)+3),QFont::Bold);
    remoteLabel->setFont(new_font);
    remoteLabel->move(curCoord.left(), curCoord.top()-(remoteLabel->fontInfo().pointSize()/3)+100);
    remoteLabel->setVisible(true);
    remoteLabel->raise();
    }

}

void TextEdit::filePrint()
{
#if QT_CONFIG(printdialog)
    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog *dlg = new QPrintDialog(&printer, this);
    if (textEdit->textCursor().hasSelection())
        dlg->addEnabledOption(QAbstractPrintDialog::PrintSelection);
    dlg->setWindowTitle(tr("Print Document"));
    if (dlg->exec() == QDialog::Accepted)
        textEdit->print(&printer);
    delete dlg;
#endif
}

void TextEdit::filePrintPreview()
{
#if QT_CONFIG(printpreviewdialog)
    QPrinter printer(QPrinter::HighResolution);
    QPrintPreviewDialog preview(&printer, this);
    connect(&preview, &QPrintPreviewDialog::paintRequested, this, &TextEdit::printPreview);
    preview.exec();
#endif
}

void TextEdit::printPreview(QPrinter *printer)
{
#ifdef QT_NO_PRINTER
    Q_UNUSED(printer);
#else
    textEdit->print(printer);
#endif
}


void TextEdit::filePrintPdf()
{
#ifndef QT_NO_PRINTER
//! [0]
    QFileDialog fileDialog(this, tr("Export PDF"));
    fileDialog.setAcceptMode(QFileDialog::AcceptSave);
    fileDialog.setMimeTypeFilters(QStringList("application/pdf"));
    fileDialog.setDefaultSuffix("pdf");
    if (fileDialog.exec() != QDialog::Accepted)
        return;
    QString fileName = fileDialog.selectedFiles().first();
    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);
    textEdit->selectAll();
    QTextEdit tmp;
    QTextCursor curs=tmp.textCursor();
    QTextCursor cu=textEdit->textCursor();
    for(unsigned long i=0;i<crdt->getSymbols().size();i++){
    cu.setPosition(i+1);
    Symbol c= crdt->getSymbols()[i];
    QTextCharFormat qform=cu.charFormat();
    qform.setBackground(Qt::transparent);
    QChar u=c.getValue();
    if(u=='\0')
        curs.insertText("\n",qform);
    else
        curs.insertText(u,qform);
}
    tmp.document()->print(&printer);
    statusBar()->showMessage(tr("Exported \"%1\"")
                             .arg(QDir::toNativeSeparators(fileName)));
//! [0]
#endif
}

void TextEdit::textBold()
{
    QTextCharFormat fmt;
    fmt.setFontWeight(actionTextBold->isChecked() ? QFont::Bold : QFont::Normal);
    mergeFormatOnWordOrSelection(fmt);
}

void TextEdit::textUnderline()
{
    QTextCharFormat fmt;
    fmt.setFontUnderline(actionTextUnderline->isChecked());
    mergeFormatOnWordOrSelection(fmt);

}

void TextEdit::textItalic()
{
    QTextCharFormat fmt;
    fmt.setFontItalic(actionTextItalic->isChecked());
    mergeFormatOnWordOrSelection(fmt);
}

void TextEdit::textFamily(const QString &f)
{
    QTextCharFormat fmt;
    fmt.setFontFamily(f);
    localFamily=f;
    flagc=true;
    mergeFormatOnWordOrSelection(fmt);
}

void TextEdit::textSize(const QString &p)
{
    qreal pointSize = p.toFloat();
    if (p.toFloat() > 0) {
        QTextCharFormat fmt;
        fmt.setFontPointSize(pointSize);
        localsize=p.toFloat();
        flagc=true;
        mergeFormatOnWordOrSelection(fmt);
    }
}

void TextEdit::textStyle(int styleIndex)
{
    QTextCursor cursor = textEdit->textCursor();
    QTextListFormat::Style style = QTextListFormat::ListStyleUndefined;

    switch (styleIndex) {
    case 1:
        style = QTextListFormat::ListDisc;
        break;
    case 2:
        style = QTextListFormat::ListCircle;
        break;
    case 3:
        style = QTextListFormat::ListSquare;
        break;
    case 4:
        style = QTextListFormat::ListDecimal;
        break;
    case 5:
        style = QTextListFormat::ListLowerAlpha;
        break;
    case 6:
        style = QTextListFormat::ListUpperAlpha;
        break;
    case 7:
        style = QTextListFormat::ListLowerRoman;
        break;
    case 8:
        style = QTextListFormat::ListUpperRoman;
        break;
    default:
        break;
    }

    cursor.beginEditBlock();

    QTextBlockFormat blockFmt = cursor.blockFormat();

    if (style == QTextListFormat::ListStyleUndefined) {
        blockFmt.setObjectIndex(-1);
        int headingLevel = styleIndex >= 9 ? styleIndex - 9 + 1 : 0; // H1 to H6, or Standard
        blockFmt.setHeadingLevel(headingLevel);
        cursor.setBlockFormat(blockFmt);

        int sizeAdjustment = headingLevel ? 4 - headingLevel : 0; // H1 to H6: +3 to -2
        QTextCharFormat fmt;
        fmt.setFontWeight(headingLevel ? QFont::Bold : QFont::Normal);
        fmt.setProperty(QTextFormat::FontSizeAdjustment, sizeAdjustment);
        cursor.select(QTextCursor::LineUnderCursor);
        cursor.mergeCharFormat(fmt);
        textEdit->mergeCurrentCharFormat(fmt);
    } else {
        QTextListFormat listFmt;
        if (cursor.currentList()) {
            listFmt = cursor.currentList()->format();
        } else {
            listFmt.setIndent(blockFmt.indent() + 1);
            blockFmt.setIndent(0);
            cursor.setBlockFormat(blockFmt);
        }
        listFmt.setStyle(style);
        cursor.createList(listFmt);
    }

    cursor.endEditBlock();
}

void TextEdit::textColor()
{
    QColor col = QColorDialog::getColor(textEdit->textColor(), this);
    if (!col.isValid())
        return;
    QTextCharFormat fmt;
    fmt.setForeground(col);
    mergeFormatOnWordOrSelection(fmt);
}

void TextEdit::textAlign(QAction *a)
{
    alignAction=true;
    if (a == actionAlignLeft)
        textEdit->setAlignment(Qt::AlignLeft | Qt::AlignAbsolute);
    else if (a == actionAlignCenter)
        textEdit->setAlignment(Qt::AlignHCenter);
    else if (a == actionAlignRight)
        textEdit->setAlignment(Qt::AlignRight | Qt::AlignAbsolute);
    else if (a == actionAlignJustify)
        textEdit->setAlignment(Qt::AlignJustify);

}

void TextEdit::showUriWindow()
{
   shu = new ShowUriDialog(this);
   shu->setUriDialog(URI);
   shu->exec();
}

void TextEdit::modifyBackground()
{
    if(backgroundOp==true){
        ereasingBackg=true;
        int pos =textEdit->textCursor().position();
        QTextCursor cursor = textEdit->textCursor();
        textEdit->selectAll();
        QTextCharFormat qform;
        qform.setBackground(Qt::transparent);
        textEdit->textCursor().mergeCharFormat(qform);
        textEdit->setTextCursor(cursor);
        textEdit->textCursor().setPosition(pos);
        backgroundOp=false;
    }
    else{
        ereasingBackg=true;
        textEdit->selectAll();
        textEdit->clear();
        QTextCursor cursor = textEdit->textCursor();
        int i=0;
        cursor.setPosition(i);
        QChar al;
        for(Symbol s : crdt->getSymbols()){

            QTextCharFormat qform;
            externAction=true;
            if(s.getSiteId()==siteid)
                qform.setBackground(Qt::transparent);
            else
                qform.setBackground(listcolor.at(s.getSiteId()%11));
            qform.setFontFamily(s.getFamily());
            qform.setFontItalic(s.getItalic());
            qform.setFontUnderline(s.getUnderln());
            qform.setFontPointSize(s.getSize());
            if(s.getBold())
                qform.setFontWeight(QFont::Bold);
            if(al!=s.getAlign()){
                al=s.getAlign();
                textEdit->setTextCursor(cursor);
                textEdit->setAlignment(insertalign(al));
                externAction=true;
            }
            cursor.insertText(s.getValue(),qform);
        }
        backgroundOp=true;
    }
    ereasingBackg=false;
    externAction=false;

}

void TextEdit::currentCharFormatChanged(const QTextCharFormat &format)
{
    fontChanged(format.font());
}

void TextEdit::onTextChanged(int position, int charsRemoved, int charsAdded)
{
    if(ereasingBackg==false){
        if(alignAction==true){
            alignAction=false;
            int max,min;
            min=position;
            max=min+charsAdded-2;
            for(int i=0;i<max;i++){
                crdt->getSymbols()[i].setAlign(findalign(textEdit->alignment()));
            }

            Message m;
            m.setAction('B');
            m.setSender(siteid);
            m.setParams({QString::number(min),QString::number(max),findalign(textEdit->alignment())});
            emit(sendTextMessage(&m));


        }
        else{
            if(externAction==false){




                QTextCursor  cursor = textEdit->textCursor();

                qDebug() << "position: " << position;
                qDebug() << "charater: " << textEdit->document()->characterAt(position).unicode();

                if(charsRemoved!=0 && charsAdded==0){
                    for(int i=0; i<charsRemoved; i++){
                        Message m=crdt->localErase(position);
                        emit(sendMessage(&m));
                    }
                }else
                    if(charsAdded!= 0){
                        if(charsAdded==charsRemoved){

                            //                dati da passare

                            int pos=cursor.selectionStart();
                            for(int i=0;i<charsAdded;i++){
                                Message mc,mi;
                                if(crdt->getSymbols().size()< pos+i+1){
                                    qDebug()<<"Errore textedit.cpp riga 760, si accede a posizione oltre nell'array symbols";
                                    qDebug()<<"ultimo carattere accedibile: " + QString(crdt->getSymbols().at(pos+i-1).getValue());
                                    pos--;
                                }
                                Symbol s=crdt->getSymbols()[pos+i];
                                //eliminazione vecchio carattere
                                mc.setSymbol(crdt->getSymbols()[pos+i]);
                                mc.setAction('D');
                                emit(sendMessage(&mc));
                                //invio carattere modificato
                                mi.setAction('I');
                                crdt->getSymbols()[pos+i].setBold(actionTextBold->isChecked());
                                crdt->getSymbols()[pos+i].setItalic(actionTextItalic->isChecked());
                                crdt->getSymbols()[pos+i].setUnderln(actionTextUnderline->isChecked());
                                crdt->getSymbols()[pos+i].setFamily(localFamily);
                                crdt->getSymbols()[pos+i].setSize(localsize);
                                crdt->getSymbols()[pos+i].setAlign(findalign(textEdit->alignment()));
                                mi.setSymbol(crdt->getSymbols()[pos+i]);
                                emit(sendMessage(&mi));
                            }
                        }
                        else if(charsRemoved!=charsAdded){
                            if(position == 0 && charsAdded > 1){
                                charsAdded--;
                                charsRemoved--;
                            }
                            int pos=cursor.selectionStart()-charsAdded;
                            for(int i=0;i<charsRemoved;i++){
                                Message mc;
                                Symbol s=crdt->getSymbols().at(pos+i);
                                //eliminazione vecchio carattere
                                mc.setSymbol(s);
                                mc.setAction('D');
                                emit(sendMessage(&mc));
                            }
                            for(int i=0;i<charsAdded;i++){
                                std::vector<Symbol>::iterator it = crdt->localInsert(textEdit->document()->characterAt(position), position-1, position);
                                Message mi;
                                if(it != this->crdt->getSymbols().end()){
                                    mi.setAction('I');
                                    it->setBold(actionTextBold->isChecked());
                                    it->setItalic(actionTextItalic->isChecked());
                                    it->setUnderln(actionTextUnderline->isChecked());
                                    it->setFamily(localFamily);
                                    it->setSize(localsize);
                                    it->setAlign(findalign(textEdit->alignment()));
                                    mi.setSymbol(*it);
                                    position+=1;
                                    emit(sendMessage(&mi));
                                }else return;
                            }
                        }
                        else{
                            if(charsRemoved>0)
                                charsAdded--;
                            for(int i=0; i<charsAdded; i++){
                                qDebug() << "char: " << textEdit->document()->characterAt(position);
                                std::vector<Symbol>::iterator it = crdt->localInsert(textEdit->document()->characterAt(position).unicode(), position-1, position);
                                Message m;
                                m.setAction('I');
                                //todo: mettere controlli
                                it->setBold(actionTextBold->isChecked());
                                it->setItalic(actionTextItalic->isChecked());
                                it->setUnderln(actionTextUnderline->isChecked());
                                it->setFamily(localFamily);
                                it->setSize(localsize);
                                it->setAlign(findalign(textEdit->alignment()));
                                m.setSymbol(*it);
                                position+=1;
                                emit(sendMessage(&m));
                            }
                        }
                    }
            }
            externAction=false;
            localOperation=true;
        }

    }
}

void TextEdit::cursorPositionChanged()
{

    QTextCursor cursor = textEdit->textCursor();
    QTextCharFormat form;
    form.setFontItalic(textEdit->currentCharFormat().fontItalic());
    form.setFontFamily(localFamily);
    form.setFontUnderline(textEdit->currentCharFormat().fontUnderline());
    form.setFontPointSize(localsize);
    form.setFontWeight(actionTextBold->isChecked() ? QFont::Bold : QFont::Normal);
    if(!cursor.hasSelection()){
        textEdit->setTextBackgroundColor(Qt::transparent);
        if(flagc==true){
            textEdit->setFontFamily(localFamily);
            textEdit->setFontPointSize(localsize);
            flagc=false;
        }
        textEdit->setFontItalic(textEdit->fontItalic());
        textEdit->setFontUnderline(textEdit->fontUnderline());
        textEdit->setFontWeight(textEdit->fontWeight());
        if(externAction){
            localOperation=false;
        }else{
            localOperation=true;
        }

    }
    alignmentChanged(textEdit->alignment());

}

void TextEdit::clipboardDataChanged()
{
#ifndef QT_NO_CLIPBOARD
    if (const QMimeData *md = QApplication::clipboard()->mimeData())
        actionPaste->setEnabled(md->hasText());
#endif
}

void TextEdit::about()
{
    QMessageBox::about(this, tr("About"), tr("This example demonstrates Qt's "
        "rich text editing facilities in action, providing an example "
        "document for you to experiment with."));
}

void TextEdit::mergeFormatOnWordOrSelection(const QTextCharFormat &format)
{
    QTextCursor cursor = textEdit->textCursor();
    if (cursor.hasSelection()){
        cursor.mergeCharFormat(format);}
    textEdit->mergeCurrentCharFormat(format);
}

void TextEdit::fontChanged(const QFont &f)
{
    comboFont->setCurrentIndex(comboFont->findText(QFontInfo(f).family()));
    comboSize->setCurrentIndex(comboSize->findText(QString::number(f.pointSize())));
    actionTextBold->setChecked(f.bold());
    actionTextItalic->setChecked(f.italic());
    actionTextUnderline->setChecked(f.underline());
}



void TextEdit::alignmentChanged(Qt::Alignment a)
{
    if (a & Qt::AlignLeft)
        actionAlignLeft->setChecked(true);
    else if (a & Qt::AlignHCenter)
        actionAlignCenter->setChecked(true);
    else if (a & Qt::AlignRight)
        actionAlignRight->setChecked(true);
    else if (a & Qt::AlignJustify)
        actionAlignJustify->setChecked(true);
}

void TextEdit::updateCursors()
{
    for (auto it = m_onlineUsers.begin(); it != m_onlineUsers.end(); it++) {
        User user;
        user = it.value();
        QRect remoteCoord = textEdit->cursorRect(user.cursor);
        int height = remoteCoord.bottom()-remoteCoord.top();
        user.label->resize(1000, height+5);

        /* update label dimension according to remote cursor position */
        QFont l_font=user.label->font();
        QTextCharFormat fmt=user.cursor.charFormat();
        int font_size=static_cast<int>(fmt.fontPointSize());
        if(font_size==0)
            font_size=DEFAULT_SIZE;
        QFont new_font(l_font.family(),static_cast<int>((font_size/2)+3),QFont::Bold);
        user.label->setFont(new_font);

        user.label->move(remoteCoord.left(), remoteCoord.top()-(user.label->fontInfo().pointSize()/3)+100);
        user.label->setVisible(true);
    }
}



void TextEdit::loadFile(QList<Symbol> file)
{
    handlingOperation = true;
    setCurrentFileName();
    QString tmp;
    QChar al;
    std::vector<Symbol> vtmp;
    QTextCursor curs=textEdit->textCursor();
    QTextCharFormat qform;
    QColor col;
    foreach(Symbol s,file){
        externAction=true;
        if(backgroundOp==true){
            if(s.getSiteId()==siteid)
                col=Qt::transparent;
            else
                col=listcolor.at(s.getSiteId()%11);
        }
        else
            col= Qt::transparent;
        qform.setBackground(col);
        qform.setFontFamily(s.getFamily());
        qform.setFontItalic(s.getItalic());
        qform.setFontUnderline(s.getUnderln());
        qform.setFontPointSize(s.getSize());
        if(s.getBold())
            qform.setFontWeight(QFont::Bold);
        if(al!=s.getAlign()){
            al=s.getAlign();
            textEdit->setTextCursor(curs);
            textEdit->setAlignment(insertalign(al));
            externAction=true;
        }
        if(s.getValue()=='\0')
            curs.insertText((QChar)'\n',qform);
        else
            curs.insertText(s.getValue(),qform);
        vtmp.push_back(s);
    }
    crdt->setSymbols(vtmp);
    updateCursors();
    handlingOperation = false;
}

Qt::Alignment TextEdit::insertalign(QChar c){
    Qt::Alignment alline;
    switch (c.toLatin1()) {
    case 'L' :
        alline=Qt::AlignLeft;
        break;
    case 'C' :
        alline= Qt::AlignHCenter;
        break;
    case 'R' :
        alline= Qt::AlignRight;
        break;
    case 'J' :
        alline= Qt::AlignJustify;
        break;
    default:
        break;
    }
    return alline;

}

QChar TextEdit::findalign(Qt::Alignment al){
    QChar c;
    if(al==Qt::AlignLeft || al==(Qt::AlignLeft | Qt::AlignAbsolute))
        c='L';
    else if(al==Qt::AlignHCenter || al==(Qt::AlignCenter | Qt::AlignAbsolute))
        c='C';
    else if(al==Qt::AlignRight || al==(Qt::AlignRight | Qt::AlignAbsolute))
        c='R';
    else if(al==Qt::AlignJustify || al==(Qt::AlignJustify | Qt::AlignAbsolute))
        c='J';
    return c;
}

void TextEdit::moveCursor(int pos, QString userId)
{
    if(pos==-24){
        if(m_onlineUsers.contains(userId)){

            delete  m_onlineUsers[userId].label;
            m_onlineUsers.remove(userId);

        }
    }
    else{
        if(m_onlineUsers.contains(userId)){
            User user;
            user = m_onlineUsers[userId];
            if(user.cursor.isNull()){
                 user.cursor = textEdit->textCursor();
            }

//            user.cursor = textEdit->textCursor();
            user.cursor.setPosition(pos); //dio errore outofrange
            QRect remoteCoord = textEdit->cursorRect(user.cursor);

            QTextCursor q; int i=0;
            q.setPosition(i);

            int height = remoteCoord.bottom()-remoteCoord.top();
            user.label->resize(1000, height+5);

            /* update label dimension according to remote cursor position */
            QFont l_font=user.label->font();
            QTextCharFormat fmt=user.cursor.charFormat();
            int font_size=static_cast<int>(fmt.fontPointSize());
            if(font_size==0)
                font_size=DEFAULT_SIZE;
            QFont new_font(l_font.family(),static_cast<int>((font_size/2)+3),QFont::Bold);
            user.label->setFont(new_font);

            user.label->move(remoteCoord.left(), remoteCoord.top()-(user.label->fontInfo().pointSize()/3)+100);
            user.label->setVisible(true);
        }
    }
}

TextEdit::~TextEdit(){
    delete this->tb;
    delete this->crdt;
    delete this->textEdit;
//    delete this->symbols;
    delete this->shu;
    delete this->m_localCursor;
}
