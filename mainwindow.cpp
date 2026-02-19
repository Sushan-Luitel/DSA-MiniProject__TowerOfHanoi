#include "mainwindow.h"
#include <QScrollArea>
#include <QStackedWidget>
#include <QFrame>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QGraphicsTextItem>
#include <QPainter>
#include <QPen>
#include <QBrush>
#include <QFont>
#include <QMouseEvent>
#include <QEvent>
#include <cmath>
#include <stack>
#include <vector>
#include <algorithm>

static const int TX[3] = {125, 375, 625};

QColor MainWindow::diskColor(int sz) {
    static QColor p[] = {
        QColor("#FF6B6B"),QColor("#FF922B"),QColor("#FCC419"),
        QColor("#51CF66"),QColor("#20C997"),QColor("#339AF0"),
        QColor("#845EF7"),QColor("#F06595")
    };
    return p[(sz-1)%8];
}
int MainWindow::towerX(int i)  { return TX[i]; }
int MainWindow::baseY()        { return SCENE_H - 45; }
int MainWindow::diskW(int sz)  {
    float r = (float)sz / game.numDisks;
    return MIN_DISK_W + (int)((MAX_DISK_W-MIN_DISK_W)*r);
}
int MainWindow::towerAtX(int x){
    for(int i=0;i<3;i++) if(std::abs(x-TX[i])<130) return i;
    return -1;
}

// ─────────────────────────────────────────────────────────────────────────────
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    numDisks(3), selectedTower(-1),
    dragging(false), dragFromTower(-1), dragGhost(nullptr),
    animating(false), animFrom(-1), animTo(-1), animDiskSz(0),
    animProgress(0), animItem(nullptr),
    animSX(0),animSY(0),animEX(0),animEY(0),animArcY(0),
    elapsedSeconds(0), gameRunning(false)
{
    setWindowTitle("Tower of Hanoi — DSA Project");
    setMinimumSize(1080, 660);

    QWidget *central = new QWidget(this);
    setCentralWidget(central);
    QHBoxLayout *root = new QHBoxLayout(central);
    root->setSpacing(14);

    // ── LEFT ─────────────────────────────────────────────────────────────────
    QVBoxLayout *leftL = new QVBoxLayout;
    leftL->setSpacing(8);

    scene = new QGraphicsScene(0, 0, SCENE_W, SCENE_H, this);
    view  = new QGraphicsView(scene, this);
    view->setRenderHint(QPainter::Antialiasing);
    view->setFixedSize(SCENE_W+2, SCENE_H+2);
    view->setStyleSheet("background:#12121F;border:2px solid #313244;border-radius:10px;");
    view->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view->installEventFilter(this);

    labelHelp = new QLabel(
        "  HOW TO PLAY:  "
        "CLICK a tower to select its top disk  →  CLICK another tower to place it  |  "
        "OR  DRAG a disk directly to another tower");
    labelHelp->setAlignment(Qt::AlignCenter);
    labelHelp->setStyleSheet(
        "color:#A6ADC8;background:#1E1E2E;font-size:11px;border-radius:6px;padding:5px;");

    labelStatus = new QLabel("Choose number of disks, then press  Reset / New Game");
    labelStatus->setAlignment(Qt::AlignCenter);
    labelStatus->setWordWrap(true);
    labelStatus->setStyleSheet(
        "font-size:13px;font-weight:bold;color:#CDD6F4;"
        "background:#313244;border-radius:8px;padding:8px;");

    leftL->addWidget(view);
    leftL->addWidget(labelHelp);
    leftL->addWidget(labelStatus);

    // ── RIGHT ─────────────────────────────────────────────────────────────────
    QVBoxLayout *rightL = new QVBoxLayout;
    rightL->setSpacing(10);

    auto mkGroup = [](const QString &t) {
        QGroupBox *g = new QGroupBox(t);
        g->setStyleSheet(
            "QGroupBox{font-weight:bold;color:#89B4FA;"
            "border:1px solid #313244;border-radius:6px;"
            "margin-top:8px;padding:6px;}");
        return g;
    };

    QGroupBox *gbInfo = mkGroup("Game Info");
    QVBoxLayout *infoL = new QVBoxLayout(gbInfo);
    labelMoveCount = new QLabel("Moves: 0");
    labelTimer     = new QLabel("Time:  00:00");
    labelMoveCount->setStyleSheet("font-size:14px;color:#89B4FA;font-weight:bold;");
    labelTimer    ->setStyleSheet("font-size:14px;color:#A6E3A1;font-weight:bold;");
    infoL->addWidget(labelMoveCount);
    infoL->addWidget(labelTimer);

    QGroupBox *gbSetup = mkGroup("Setup");
    QHBoxLayout *setupL = new QHBoxLayout(gbSetup);
    QLabel *lbD = new QLabel("Number of Disks:");
    lbD->setStyleSheet("color:#CDD6F4;");
    comboDiskCount = new QComboBox;
    for(int i=2;i<=8;i++) comboDiskCount->addItem(QString::number(i));
    comboDiskCount->setCurrentIndex(1);
    comboDiskCount->setStyleSheet(
        "background:#313244;color:#CDD6F4;padding:4px;border-radius:4px;");
    setupL->addWidget(lbD);
    setupL->addWidget(comboDiskCount);

    auto mkBtn=[](const QString &txt,const QString &bg){
        QPushButton *b=new QPushButton(txt);
        b->setCursor(Qt::PointingHandCursor);
        b->setStyleSheet(QString(
                             "QPushButton{background:%1;color:white;font-weight:bold;"
                             "padding:10px;border-radius:7px;font-size:13px;}"
                             "QPushButton:pressed{padding:9px 11px;}").arg(bg));
        return b;
    };
    btnAutoSolve = mkBtn("⚡  Auto Solve (Queue)","#7C3AED");
    btnUndo      = mkBtn("↩  Undo Last Move",     "#D97706");
    btnReset     = mkBtn("🔄  Reset / New Game",  "#059669");
    btnAbout     = mkBtn("📖  About / DSA Info",   "#1e6ba1");

    QGroupBox *gbLog = mkGroup("Move History (Queue log)");
    QVBoxLayout *logL = new QVBoxLayout(gbLog);
    moveLogList = new QListWidget;
    moveLogList->setMinimumHeight(200);
    moveLogList->setStyleSheet(
        "QListWidget{background:#12121F;color:#CDD6F4;"
        "font-size:12px;border:none;border-radius:4px;}"
        "QListWidget::item{padding:2px;}");
    logL->addWidget(moveLogList);

    rightL->addWidget(gbInfo);
    rightL->addWidget(gbSetup);
    rightL->addWidget(btnAutoSolve);
    rightL->addWidget(btnUndo);
    rightL->addWidget(btnReset);
    rightL->addWidget(btnAbout);
    rightL->addWidget(gbLog);
    rightL->addStretch();

    root->addLayout(leftL);
    root->addLayout(rightL);
    setStyleSheet("QMainWindow,QWidget{background:#181825;}");

    // Animation timer — fires 60fps
    animTimer = new QTimer(this);
    animTimer->setInterval(16);
    connect(animTimer, &QTimer::timeout, this, &MainWindow::onAnimStep);

    autoSolveTimer = new QTimer(this);
    autoSolveTimer->setInterval(700);
    connect(autoSolveTimer, &QTimer::timeout, this, &MainWindow::onAutoSolveStep);

    clockTimer = new QTimer(this);
    clockTimer->setInterval(1000);
    connect(clockTimer, &QTimer::timeout, this, &MainWindow::onTimerTick);

    connect(btnAutoSolve, &QPushButton::clicked, this, &MainWindow::onAutoSolveClicked);
    connect(btnUndo,      &QPushButton::clicked, this, &MainWindow::onUndoClicked);
    connect(btnReset,     &QPushButton::clicked, this, &MainWindow::onResetClicked);
    connect(btnAbout,     &QPushButton::clicked, this, &MainWindow::onAboutClicked);

    game.init(3);
    redraw();
}

MainWindow::~MainWindow() {}

// ─── Event filter ─────────────────────────────────────────────────────────────
bool MainWindow::eventFilter(QObject *obj, QEvent *ev) {
    if(obj != view) return QMainWindow::eventFilter(obj,ev);
    if(animating)   return true;  // block input while animating

    if(ev->type()==QEvent::MouseButtonPress){
        QMouseEvent *me = static_cast<QMouseEvent*>(ev);
        handleMousePress(view->mapToScene(me->pos()));
        return true;
    }
    if(ev->type()==QEvent::MouseMove){
        QMouseEvent *me = static_cast<QMouseEvent*>(ev);
        handleMouseMove(view->mapToScene(me->pos()));
        return true;
    }
    if(ev->type()==QEvent::MouseButtonRelease){
        QMouseEvent *me = static_cast<QMouseEvent*>(ev);
        handleMouseRelease(view->mapToScene(me->pos()));
        return true;
    }
    return QMainWindow::eventFilter(obj,ev);
}

// ─── Mouse ────────────────────────────────────────────────────────────────────
void MainWindow::handleMousePress(QPointF sp) {
    if(autoSolveTimer->isActive()) return;
    int tIdx = towerAtX((int)sp.x());
    if(tIdx==-1) return;

    QString names[3]={"A","B","C"};
    Tower *t = game.getTower(names[tIdx].toStdString());

    // Start drag only if tower has disks
    if(t && !t->isEmpty()){
        dragging=true;
        dragFromTower=tIdx;
        int sz=t->top();
        int dw=diskW(sz);
        QColor col=diskColor(sz);
        // ghost follows cursor
        dragGhost=scene->addRect(-dw/2,-(DISK_H-4)/2,dw,DISK_H-4,
                                   QPen(col.lighter(140),2),
                                   QBrush(QColor(col.red(),col.green(),col.blue(),150)));
        dragGhost->setPos(sp);
        dragGhost->setZValue(200);
    }

    // Also run click-select logic
    handleTowerClick(tIdx);
}

void MainWindow::handleMouseMove(QPointF sp){
    if(dragging && dragGhost)
        dragGhost->setPos(sp);
}

void MainWindow::handleMouseRelease(QPointF sp){
    if(!dragging) return;
    dragging=false;

    // safely remove ghost
    if(dragGhost){
        scene->removeItem(dragGhost);
        dragGhost=nullptr;
    }

    int toTower=towerAtX((int)sp.x());
    if(toTower!=-1 && toTower!=dragFromTower){
        // drag completed to different tower
        selectedTower=-1;
        doMove(dragFromTower, toTower);
    }
    dragFromTower=-1;
}

// ─── Click select/place ───────────────────────────────────────────────────────
void MainWindow::handleTowerClick(int tIdx){
    if(autoSolveTimer->isActive()) return;
    QString names[3]={"A","B","C"};

    if(selectedTower==-1){
        Tower *t=game.getTower(names[tIdx].toStdString());
        if(!t||t->isEmpty()){
            updateStatus("That tower is empty! Click a tower that has disks.");
            return;
        }
        selectedTower=tIdx;
        redraw();
        updateStatus(QString("Tower %1 selected — now click the destination tower  (click same tower to cancel)").arg(names[tIdx]));
    } else {
        if(tIdx==selectedTower){
            selectedTower=-1; redraw();
            updateStatus("Selection cancelled.");
            return;
        }
        int from=selectedTower;
        selectedTower=-1;
        doMove(from,tIdx);
    }
}

// ─── Move + Animation ─────────────────────────────────────────────────────────
void MainWindow::doMove(int from, int to){
    QString names[3]={"A","B","C"};

    Tower *src=game.getTower(names[from].toStdString());
    Tower *dst=game.getTower(names[to].toStdString());

    if(!src||src->isEmpty()){
        updateStatus("That tower is empty!"); return;
    }
    int diskSz=src->top();
    if(!dst->isEmpty()&&dst->top()<diskSz){
        updateStatus("Invalid! Cannot place a larger disk on a smaller one.");
        redraw(); return;
    }

    if(!gameRunning){ gameRunning=true; clockTimer->start(); }

    // --- Setup animation ---
    int srcCount=(int)src->disks.size();
    int dstCount=(int)dst->disks.size();

    animFrom    = from;
    animTo      = to;
    animDiskSz  = diskSz;
    animProgress= 0.0f;
    animSX      = towerX(from);
    animSY      = baseY() - srcCount * DISK_H;       // top disk Y on source
    animEX      = towerX(to);
    animEY      = baseY() - (dstCount+1) * DISK_H;   // landing Y on dest
    animArcY    = 55.0f;                              // arc peak Y

    // Create the moving disk rectangle
    int dw = diskW(diskSz);
    QColor col = diskColor(diskSz);
    animItem = scene->addRect(-dw/2, -(DISK_H-4)/2, dw, DISK_H-4,
                              QPen(col.darker(140),2), QBrush(col));
    animItem->setPos(animSX, animSY);
    animItem->setZValue(150);

    animating = true;
    animTimer->start();

    // Hide the disk on source while it animates by redrawing
    // We temporarily pop it from the game for visual only - NO, just redraw towers/disks
    // hiding top disk of source: easiest = just redraw without changing game state
    // The animItem visually replaces it. Redraw scene (clear disks except anim item)
    scene->clear();
    animItem = scene->addRect(-dw/2, -(DISK_H-4)/2, dw, DISK_H-4,
                              QPen(col.darker(140),2), QBrush(col));
    animItem->setPos(animSX, animSY);
    animItem->setZValue(150);
    drawTowers();
    // Draw all disks EXCEPT the top disk on source tower
    drawDisksExcept(from, diskSz);
}

void MainWindow::drawDisksExcept(int skipTower, int skipTopSz){
    Q_UNUSED(skipTopSz);
    auto drawFor=[&](Tower &t, int tIdx, bool skipTop){
        std::stack<int> tmp=t.disks;
        std::vector<int> lst;
        while(!tmp.empty()){lst.push_back(tmp.top());tmp.pop();}
        std::reverse(lst.begin(),lst.end());

        int cx=towerX(tIdx);
        int by=baseY();
        int count=skipTop?(int)lst.size()-1:(int)lst.size();

        for(int i=0;i<count;i++){
            int sz=lst[i];
            int dw=diskW(sz);
            int dy=by-(i+1)*DISK_H;
            QColor col=diskColor(sz);

            scene->addRect(cx-dw/2+3,dy+3,dw,DISK_H-4,
                           QPen(Qt::NoPen),QBrush(QColor(0,0,0,90)));
            scene->addRect(cx-dw/2,dy,dw,DISK_H-4,
                           QPen(col.darker(140),1.5),QBrush(col));
            scene->addRect(cx-dw/2+4,dy+2,dw-8,5,
                           QPen(Qt::NoPen),QBrush(QColor(255,255,255,55)));
            QGraphicsTextItem *lbl=scene->addText(QString::number(sz));
            lbl->setDefaultTextColor(QColor("#1E1E2E"));
            lbl->setFont(QFont("Arial",9,QFont::Bold));
            lbl->setPos(cx-5,dy+4);
        }
    };
    drawFor(game.towerA,0,skipTower==0);
    drawFor(game.towerB,1,skipTower==1);
    drawFor(game.towerC,2,skipTower==2);
}

// Called every 16ms during animation
void MainWindow::onAnimStep(){
    if(!animating||!animItem) return;

    animProgress += 0.04f;  // completes in ~25 frames (~400ms)
    if(animProgress>=1.0f) animProgress=1.0f;

    float t=animProgress;
    // Arc: use quadratic bezier with control point at top
    // P = (1-t)^2 * start + 2*(1-t)*t * mid + t^2 * end
    float midX=(animSX+animEX)/2.0f;
    float midY=animArcY;
    float x=(1-t)*(1-t)*animSX + 2*(1-t)*t*midX + t*t*animEX;
    float y=(1-t)*(1-t)*animSY + 2*(1-t)*t*midY  + t*t*animEY;

    animItem->setPos(x,y);

    if(animProgress>=1.0f){
        animTimer->stop();
        animating=false;
        // Remove anim item before redraw
        scene->removeItem(animItem);
        delete animItem;
        animItem=nullptr;
        // Now execute actual move in game logic
        finishMove(animFrom,animTo);
    }
}

void MainWindow::finishMove(int from, int to){
    QString names[3]={"A","B","C"};
    game.moveDisk(names[from].toStdString(),names[to].toStdString());
    updateMoveLog();
    redraw();
    checkWin();
}

void MainWindow::checkWin(){
    if(!game.isWon()) return;
    clockTimer->stop();
    gameRunning=false;
    int mn=(int)std::pow(2,game.numDisks)-1;
    updateStatus(QString("YOU WON!  Moves: %1  |  Minimum: %2  |  Time: %3s")
                     .arg(game.moveCount).arg(mn).arg(elapsedSeconds));
    QTimer::singleShot(300,this,&MainWindow::showWinDialog);
}

// ─── Win dialog ───────────────────────────────────────────────────────────────
void MainWindow::showWinDialog(){
    int mn   =(int)std::pow(2,game.numDisks)-1;
    int extra=game.moveCount-mn;
    QString rating=extra==0?"PERFECT — Minimum moves!":
                         extra<=3?"Excellent!":
                         extra<=10?"Good job!":"Keep practising!";

    QDialog dlg(this);
    dlg.setWindowTitle("Congratulations!");
    dlg.setFixedSize(420,380);
    dlg.setStyleSheet("QDialog{background:#1E1E2E;}");

    QVBoxLayout vl(&dlg);
    vl.setSpacing(12);
    vl.setContentsMargins(28,20,28,20);

    auto mk=[&](const QString &txt,const QString &css){
        QLabel *l=new QLabel(txt);
        l->setAlignment(Qt::AlignCenter);
        l->setStyleSheet(css+"background:transparent;");
        return l;
    };

    vl.addWidget(mk("🏆","font-size:60px;"));
    vl.addWidget(mk("CONGRATULATIONS!",
                    "font-size:21px;font-weight:bold;color:#F5A623;letter-spacing:2px;"));
    vl.addWidget(mk("You solved Tower of Hanoi!",
                    "font-size:13px;color:#CDD6F4;"));

    // stats box
    QFrame *box=new QFrame;
    box->setStyleSheet("background:#313244;border-radius:10px;");
    QVBoxLayout *bl=new QVBoxLayout(box);
    bl->setSpacing(6);
    bl->setContentsMargins(16,12,16,12);

    auto stat=[&](const QString &ico,const QString &label,const QString &val,const QString &col){
        QLabel *l=new QLabel(
            QString("%1  %2  <span style='color:%3;font-weight:bold;font-size:14px;'>%4</span>")
                .arg(ico).arg(label).arg(col).arg(val));
        l->setStyleSheet("font-size:13px;color:#CDD6F4;background:transparent;");
        l->setTextFormat(Qt::RichText);
        bl->addWidget(l);
    };
    stat("🎯","Your moves:    ",QString::number(game.moveCount),"#FF922B");
    stat("⚡","Minimum moves: ",QString::number(mn),           "#51CF66");
    stat("⏱","Time taken:    ",QString("%1s").arg(elapsedSeconds),"#339AF0");
    stat("⭐","Rating:        ",rating,                         "#FCC419");
    vl.addWidget(box);

    QHBoxLayout *br=new QHBoxLayout;
    auto mkB=[&](const QString &txt,const QString &bg){
        QPushButton *b=new QPushButton(txt);
        b->setStyleSheet(QString(
                             "QPushButton{background:%1;color:white;font-weight:bold;"
                             "padding:10px;border-radius:8px;font-size:13px;}"
                             "QPushButton:pressed{padding:9px;}").arg(bg));
        return b;
    };
    QPushButton *bPlay =mkB("🔄  Play Again","#059669");
    QPushButton *bClose=mkB("Close",         "#45475A");
    connect(bPlay, &QPushButton::clicked, &dlg, &QDialog::accept);
    connect(bClose,&QPushButton::clicked, &dlg, &QDialog::reject);
    br->addWidget(bPlay);
    br->addWidget(bClose);
    vl.addLayout(br);

    dlg.exec();

    // If Play Again was clicked
    if(dlg.result()==QDialog::Accepted)
        onResetClicked();
}

// ─── Draw ─────────────────────────────────────────────────────────────────────
void MainWindow::redraw(){
    scene->clear();
    animItem=nullptr;
    dragGhost=nullptr;
    drawTowers();
    drawDisks();
}

void MainWindow::drawTowers(){
    scene->setBackgroundBrush(QBrush(QColor("#12121F")));

    QGraphicsTextItem *ttl=scene->addText("TOWER  OF  HANOI");
    ttl->setDefaultTextColor(QColor("#89B4FA"));
    ttl->setFont(QFont("Arial",15,QFont::Bold));
    ttl->setPos(SCENE_W/2-118,6);

    const QString lbs[3]  ={"A","B","C"};
    const QString hts[3]  ={"SOURCE","AUXILIARY","DESTINATION"};

    for(int i=0;i<3;i++){
        int cx=towerX(i),by=baseY();
        bool sel=(i==selectedTower);

        if(sel)
            scene->addRect(cx-MAX_DISK_W/2-8,by-ROD_H,MAX_DISK_W+16,ROD_H+BASE_H,
                           QPen(QColor("#89B4FA"),2,Qt::DashLine),
                           QBrush(QColor(137,180,250,22)));

        scene->addRect(cx-MAX_DISK_W/2-8,by,MAX_DISK_W+16,BASE_H,
                       QPen(Qt::NoPen),
                       QBrush(sel?QColor("#585B70"):QColor("#3A3B50")));
        scene->addRect(cx-6,by-ROD_H,12,ROD_H,
                       QPen(Qt::NoPen),
                       QBrush(sel?QColor("#89B4FA"):QColor("#585B70")));

        QGraphicsTextItem *nm=scene->addText(lbs[i]);
        nm->setDefaultTextColor(sel?QColor("#F5A623"):QColor("#CDD6F4"));
        nm->setFont(QFont("Arial",20,QFont::Bold));
        nm->setPos(cx-12,by+BASE_H+2);

        QGraphicsTextItem *hn=scene->addText(hts[i]);
        hn->setDefaultTextColor(QColor("#585B70"));
        hn->setFont(QFont("Arial",8));
        hn->setPos(cx-36,by+BASE_H+28);

        if(sel){
            QGraphicsTextItem *b=scene->addText("SELECTED");
            b->setDefaultTextColor(QColor("#F5A623"));
            b->setFont(QFont("Arial",9,QFont::Bold));
            b->setPos(cx-28,by-ROD_H-20);
        }
    }
}

void MainWindow::drawDisks(){
    auto drawFor=[&](Tower &t,int tIdx){
        std::stack<int> tmp=t.disks;
        std::vector<int> lst;
        while(!tmp.empty()){lst.push_back(tmp.top());tmp.pop();}
        std::reverse(lst.begin(),lst.end());

        int cx=towerX(tIdx),by=baseY();
        bool sel=(tIdx==selectedTower);

        for(int i=0;i<(int)lst.size();i++){
            int sz=lst[i];
            int dw=diskW(sz);
            int dy=by-(i+1)*DISK_H;
            QColor col=diskColor(sz);
            bool topSel=sel&&(i==(int)lst.size()-1);

            scene->addRect(cx-dw/2+3,dy+3,dw,DISK_H-4,
                           QPen(Qt::NoPen),QBrush(QColor(0,0,0,90)));
            scene->addRect(cx-dw/2,dy,dw,DISK_H-4,
                           QPen(topSel?QColor("#F5A623"):col.darker(140),topSel?2.5f:1.5f),
                           QBrush(topSel?col.lighter(120):col));
            scene->addRect(cx-dw/2+4,dy+2,dw-8,5,
                           QPen(Qt::NoPen),QBrush(QColor(255,255,255,55)));
            if(topSel)
                scene->addRect(cx-dw/2-4,dy-4,dw+8,DISK_H+4,
                               QPen(QColor("#F5A623"),2.5),QBrush(Qt::NoBrush));

            QGraphicsTextItem *lbl=scene->addText(QString::number(sz));
            lbl->setDefaultTextColor(QColor("#1E1E2E"));
            lbl->setFont(QFont("Arial",9,QFont::Bold));
            lbl->setPos(cx-5,dy+4);
        }
    };
    drawFor(game.towerA,0);
    drawFor(game.towerB,1);
    drawFor(game.towerC,2);
}

// ─── Slots ────────────────────────────────────────────────────────────────────
void MainWindow::onAutoSolveClicked(){
    if(animating) return;
    if(autoSolveTimer->isActive()){
        autoSolveTimer->stop();
        btnAutoSolve->setText("⚡  Auto Solve (Queue)");
        return;
    }
    while(!game.solutionQueue.empty()) game.solutionQueue.pop();
    game.generateSolution(game.numDisks,"A","B","C");
    selectedTower=-1;
    if(!gameRunning){gameRunning=true;clockTimer->start();}
    btnAutoSolve->setText("⏸  Pause");
    autoSolveTimer->start();
    updateStatus("Auto-solving...");
}

void MainWindow::onAutoSolveStep(){
    if(animating) return;  // wait for animation to finish
    if(game.solutionQueue.empty()){
        autoSolveTimer->stop();
        clockTimer->stop();
        gameRunning=false;
        btnAutoSolve->setText("⚡  Auto Solve (Queue)");
        redraw();
        updateStatus(QString("Auto-solve done!  Moves: %1").arg(game.moveCount));
        return;
    }
    Move m=game.solutionQueue.front();
    game.solutionQueue.pop();
    auto ni=[](const std::string &n){return n=="A"?0:n=="B"?1:2;};
    doMove(ni(m.from),ni(m.to));
}

void MainWindow::onUndoClicked(){
    if(animating||autoSolveTimer->isActive()) return;
    selectedTower=-1;
    game.undoMove();
    redraw();
    updateMoveLog();
    updateStatus("Undo — last move reversed.");
}

void MainWindow::onResetClicked(){
    animTimer->stop();
    autoSolveTimer->stop();
    clockTimer->stop();
    animating=false; animItem=nullptr;
    btnAutoSolve->setText("⚡  Auto Solve (Queue)");
    gameRunning=false; elapsedSeconds=0; selectedTower=-1;
    dragging=false; dragFromTower=-1; dragGhost=nullptr;
    labelTimer->setText("Time:  00:00");
    numDisks=comboDiskCount->currentText().toInt();
    game.reset(numDisks);
    redraw();
    updateMoveLog();
    updateStatus("Game reset!  Click a tower to select a disk, then click where to place it.");
}

void MainWindow::onTimerTick(){
    elapsedSeconds++;
    int m=elapsedSeconds/60,s=elapsedSeconds%60;
    labelTimer->setText(QString("Time:  %1:%2")
                            .arg(m,2,10,QChar('0')).arg(s,2,10,QChar('0')));
}

void MainWindow::updateMoveLog(){
    moveLogList->clear();
    for(const auto &s:game.moveLog)
        moveLogList->addItem(QString::fromStdString(s));
    moveLogList->scrollToBottom();
    labelMoveCount->setText(QString("Moves: %1").arg(game.moveCount));
}
void MainWindow::updateStatus(const QString &msg){labelStatus->setText(msg);}

// ─── About / DSA Info Dialog ──────────────────────────────────────────────────
void MainWindow::onAboutClicked() {
    showAboutDialog();
}

void MainWindow::showAboutDialog() {
    QDialog dlg(this);
    dlg.setWindowTitle("About — Tower of Hanoi & DSA");
    dlg.setFixedSize(780, 620);
    dlg.setStyleSheet("QDialog { background:#12121F; }");

    QVBoxLayout *mainL = new QVBoxLayout(&dlg);
    mainL->setContentsMargins(0,0,0,0);
    mainL->setSpacing(0);

    // ── Top header bar ────────────────────────────────────────────────────────
    QWidget *header = new QWidget;
    header->setFixedHeight(90);
    header->setStyleSheet(
        "background: qlineargradient(x1:0,y1:0,x2:1,y2:0,"
        "stop:0 #1a1a3e, stop:0.5 #2d1b4e, stop:1 #1a2a4e);"
        "border-bottom: 2px solid #313244;");
    QHBoxLayout *headerL = new QHBoxLayout(header);
    headerL->setContentsMargins(24,0,24,0);

    QLabel *dlgTitle = new QLabel("🗼  Tower of Hanoi — DSA Implementation Guide");
    dlgTitle->setStyleSheet(
        "font-size:17px; font-weight:bold; color:#CDD6F4; background:transparent;");

    QLabel *dlgSub = new QLabel("How Stack · Queue · Tree are used in this project");
    dlgSub->setStyleSheet(
        "font-size:11px; color:#6C7086; background:transparent; margin-top:4px;");

    QVBoxLayout *titleCol = new QVBoxLayout;
    titleCol->addWidget(dlgTitle);
    titleCol->addWidget(dlgSub);
    headerL->addLayout(titleCol);
    headerL->addStretch();
    mainL->addWidget(header);

    // ── Tab bar ───────────────────────────────────────────────────────────────
    QWidget *tabBar = new QWidget;
    tabBar->setFixedHeight(46);
    tabBar->setStyleSheet("background:#1E1E2E; border-bottom:1px solid #313244;");
    QHBoxLayout *tabL = new QHBoxLayout(tabBar);
    tabL->setContentsMargins(16,0,16,0);
    tabL->setSpacing(4);

    struct Tab { QString label; QString color; };
    Tab tabs[] = {
        {"📚  Stack",   "#fb7185"},
        {"🚶  Queue",   "#4ea8de"},
        {"🌳  Tree",    "#fcc419"},
        {"ℹ️  About Game", "#a6e3a1"}
    };

    QStackedWidget *pages = new QStackedWidget;
    pages->setStyleSheet("background:#12121F;");

    QList<QPushButton*> tabBtns;
    for(int i=0;i<4;i++){
        QPushButton *tb = new QPushButton(tabs[i].label);
        tb->setCheckable(true);
        tb->setFixedHeight(34);
        QString col = tabs[i].color;
        tb->setStyleSheet(QString(
                              "QPushButton{background:transparent;color:#6C7086;font-size:12px;"
                              "font-weight:bold;border:none;border-radius:6px;padding:0 14px;}"
                              "QPushButton:checked{background:rgba(255,255,255,0.07);color:%1;"
                              "border-bottom:2px solid %1;}").arg(col));
        tabL->addWidget(tb);
        tabBtns.append(tb);
    }
    tabL->addStretch();
    mainL->addWidget(tabBar);
    mainL->addWidget(pages);

    // ── Helper: styled label ──────────────────────────────────────────────────
    auto mkLabel = [](const QString &txt, const QString &css) -> QLabel* {
        QLabel *l = new QLabel(txt);
        l->setStyleSheet(css);
        l->setWordWrap(true);
        l->setTextFormat(Qt::RichText);
        return l;
    };

    auto mkCode = [](const QString &code) -> QWidget* {
        QWidget *box = new QWidget;
        box->setStyleSheet(
            "background:#0d0d1a; border:1px solid #313244; border-radius:10px;");
        QVBoxLayout *bl = new QVBoxLayout(box);
        bl->setContentsMargins(0,0,0,0);
        bl->setSpacing(0);

        QWidget *cHeader = new QWidget;
        cHeader->setFixedHeight(30);
        cHeader->setStyleSheet(
            "background:rgba(255,255,255,0.04);"
            "border-bottom:1px solid #313244;"
            "border-radius:10px 10px 0 0;");
        QHBoxLayout *ch = new QHBoxLayout(cHeader);
        ch->setContentsMargins(10,0,10,0);
        for(auto c:{"#ff5f57","#febc2e","#28c840"}){
            QLabel *dot=new QLabel;
            dot->setFixedSize(10,10);
            dot->setStyleSheet(QString("background:%1;border-radius:5px;").arg(c));
            ch->addWidget(dot);
        }
        ch->addStretch();
        QLabel *lang=new QLabel("C++");
        lang->setStyleSheet("color:#585b70;font-size:10px;font-family:'Courier New';");
        ch->addWidget(lang);
        bl->addWidget(cHeader);

        QLabel *pre = new QLabel(code);
        pre->setStyleSheet(
            "color:#cdd6f4; font-family:'Courier New',monospace;"
            "font-size:11px; padding:14px 16px; line-height:1.6;"
            "background:transparent;");
        pre->setWordWrap(false);
        pre->setTextFormat(Qt::PlainText);
        bl->addWidget(pre);
        return box;
    };

    auto mkSection = [&](const QString &icon, const QString &title,
                         const QString &col, const QString &badge) -> QWidget* {
        QWidget *w = new QWidget;
        QHBoxLayout *hl = new QHBoxLayout(w);
        hl->setContentsMargins(0,0,0,0);
        QLabel *ic = new QLabel(icon);
        ic->setStyleSheet("font-size:22px;background:transparent;");
        QLabel *ti = new QLabel(title);
        ti->setStyleSheet(QString("font-size:16px;font-weight:bold;color:%1;background:transparent;").arg(col));
        QLabel *bd = new QLabel(badge);
        bd->setStyleSheet(QString(
                              "font-size:10px;color:%1;background:rgba(255,255,255,0.06);"
                              "border:1px solid %1;border-radius:999px;padding:2px 10px;"
                              "margin-left:8px;").arg(col));
        hl->addWidget(ic);
        hl->addWidget(ti);
        hl->addWidget(bd);
        hl->addStretch();
        return w;
    };

    // ═══════════════════════════════════════════════════════
    // PAGE 1 — STACK
    // ═══════════════════════════════════════════════════════
    QWidget *pgStack = new QWidget;
    pgStack->setStyleSheet("background:#12121F;");
    QScrollArea *saStack = new QScrollArea;
    saStack->setWidget(pgStack);
    saStack->setWidgetResizable(true);
    saStack->setStyleSheet("QScrollArea{border:none;background:#12121F;}"
                           "QScrollBar:vertical{width:4px;background:#1E1E2E;}"
                           "QScrollBar::handle:vertical{background:#313244;border-radius:2px;}");

    QVBoxLayout *slL = new QVBoxLayout(pgStack);
    slL->setContentsMargins(24,20,24,20);
    slL->setSpacing(14);

    slL->addWidget(mkSection("📚","Stack","#fb7185","LIFO — Last In, First Out"));

    slL->addWidget(mkLabel(
        "<span style='color:#CDD6F4;font-size:13px;'>"
        "Each of the three towers (A, B, C) is implemented as a "
        "<span style='color:#fb7185;font-family:Courier New;'>std::stack&lt;int&gt;</span>. "
        "The integer stored is the disk size. Stack enforces LIFO — you can only touch the TOP disk, "
        "which perfectly matches the rule that you can only move the top disk of any tower."
        "</span>",
        "background:transparent;"));

    // Visual disk stack
    QWidget *vizStack = new QWidget;
    vizStack->setStyleSheet(
        "background:#1E1E2E; border:1px solid #313244; border-radius:12px;");
    vizStack->setFixedHeight(180);
    QHBoxLayout *vizL = new QHBoxLayout(vizStack);
    vizL->setAlignment(Qt::AlignCenter);
    vizL->setSpacing(40);

    struct DiskViz { QString col; int w; QString lbl; };
    DiskViz disks3[] = {{"#fcc419",60,"1"},{"#ff922b",80,"2"},{"#ff6b6b",100,"3"}};

    QWidget *towerVizW = new QWidget;
    QVBoxLayout *tvL = new QVBoxLayout(towerVizW);
    tvL->setSpacing(3); tvL->setAlignment(Qt::AlignHCenter);

    QLabel *topArrow = new QLabel("← push() / pop() here");
    topArrow->setStyleSheet("color:#fb7185;font-size:10px;font-family:'Courier New';background:transparent;");
    tvL->addWidget(topArrow);

    for(auto &d : disks3){
        QLabel *disk = new QLabel(QString("  disk %1  ").arg(d.lbl));
        disk->setFixedSize(d.w, 24);
        disk->setAlignment(Qt::AlignCenter);
        disk->setStyleSheet(QString(
                                "background:%1;border-radius:5px;color:#1E1E2E;"
                                "font-size:11px;font-weight:bold;font-family:'Courier New';").arg(d.col));
        tvL->addWidget(disk, 0, Qt::AlignHCenter);
    }
    QLabel *base = new QLabel;
    base->setFixedSize(120,8);
    base->setStyleSheet("background:#45475A;border-radius:4px;");
    tvL->addWidget(base, 0, Qt::AlignHCenter);
    QLabel *baseLabel = new QLabel("Tower A  (std::stack)");
    baseLabel->setStyleSheet("color:#6C7086;font-size:10px;font-family:'Courier New';background:transparent;");
    tvL->addWidget(baseLabel, 0, Qt::AlignHCenter);

    vizL->addWidget(towerVizW);

    // Operations list
    QWidget *opsW = new QWidget;
    opsW->setStyleSheet("background:transparent;");
    QVBoxLayout *opsL = new QVBoxLayout(opsW);
    opsL->setSpacing(8);
    struct Op { QString fn; QString desc; };
    Op ops[] = {
                {"push(disk)","place disk on top of tower"},
                {"pop()","lift top disk off tower"},
                {"top()","peek at top disk (validation)"},
                {"empty()","check if tower is empty"},
                };
    for(auto &op : ops){
        QLabel *ol = new QLabel(
            QString("<span style='color:#fb7185;font-family:Courier New;font-size:12px;'>%1</span>"
                    "  <span style='color:#6C7086;font-size:11px;'>— %2</span>")
                .arg(op.fn).arg(op.desc));
        ol->setStyleSheet("background:transparent;");
        ol->setTextFormat(Qt::RichText);
        opsL->addWidget(ol);
    }
    vizL->addWidget(opsW);
    slL->addWidget(vizStack);

    slL->addWidget(mkCode(
        "// Each tower is a Stack\n"
        "std::stack<int> towerA, towerB, towerC;\n\n"
        "// Move disk from src to dst\n"
        "bool moveDisk(Tower& src, Tower& dst) {\n"
        "    if (src.isEmpty()) return false;\n"
        "    int disk = src.top();   // peek\n"
        "    // Validate: no big on small\n"
        "    if (!dst.isEmpty() && dst.top() < disk)\n"
        "        return false;       // INVALID\n"
        "    src.pop();              // lift disk\n"
        "    dst.push(disk);         // place disk\n"
        "    return true;\n"
        "}\n\n"
        "// Undo uses a second Stack\n"
        "std::stack<Move> undoStack;\n"
        "undoStack.push({from, to, disk}); // on every move\n"
        "// Undo: pop from undoStack, reverse it"
        ));

    slL->addStretch();
    pages->addWidget(saStack);

    // ═══════════════════════════════════════════════════════
    // PAGE 2 — QUEUE
    // ═══════════════════════════════════════════════════════
    QWidget *pgQueue = new QWidget;
    pgQueue->setStyleSheet("background:#12121F;");
    QScrollArea *saQueue = new QScrollArea;
    saQueue->setWidget(pgQueue);
    saQueue->setWidgetResizable(true);
    saQueue->setStyleSheet("QScrollArea{border:none;background:#12121F;}"
                           "QScrollBar:vertical{width:4px;background:#1E1E2E;}"
                           "QScrollBar::handle:vertical{background:#313244;border-radius:2px;}");

    QVBoxLayout *qlL = new QVBoxLayout(pgQueue);
    qlL->setContentsMargins(24,20,24,20);
    qlL->setSpacing(14);

    qlL->addWidget(mkSection("🚶","Queue","#4ea8de","FIFO — First In, First Out"));

    qlL->addWidget(mkLabel(
        "<span style='color:#CDD6F4;font-size:13px;'>"
        "When you press <b>Auto Solve</b>, the recursive algorithm runs instantly and fills a "
        "<span style='color:#4ea8de;font-family:Courier New;'>std::queue&lt;Move&gt;</span> "
        "with every required move in correct order. The animation timer then dequeues one move "
        "per tick — guaranteeing moves execute in the exact right sequence. "
        "The Move History log you see on the right is also a queue-ordered list."
        "</span>",
        "background:transparent;"));

    // ── Queue Visual — full correct diagram ─────────────────────────────────
    // For n=3: 7 moves.  Show all 7 steps clearly.
    QWidget *qViz = new QWidget;
    qViz->setStyleSheet(
        "background:#1E1E2E; border:1px solid #313244; border-radius:12px;");
    QVBoxLayout *qvL2 = new QVBoxLayout(qViz);
    qvL2->setContentsMargins(16,14,16,14);
    qvL2->setSpacing(10);

    // Title row
    QLabel *qTitle = new QLabel(
        "  solutionQueue  for n = 3 disks  →  7 moves total  (2³ − 1 = 7)");
    qTitle->setStyleSheet(
        "color:#4ea8de; font-size:11px; font-weight:bold;"
        "font-family:'Courier New'; background:transparent;");
    qvL2->addWidget(qTitle);

    // Direction labels
    QHBoxLayout *dirRow = new QHBoxLayout;
    QLabel *deqLbl = new QLabel("dequeue() ← FRONT");
    deqLbl->setStyleSheet(
        "color:#fb7185; font-size:10px; font-family:'Courier New'; background:transparent;");
    QLabel *enqLbl = new QLabel("BACK → enqueue()");
    enqLbl->setStyleSheet(
        "color:#a6e3a1; font-size:10px; font-family:'Courier New'; background:transparent;");
    dirRow->addWidget(deqLbl);
    dirRow->addStretch();
    dirRow->addWidget(enqLbl);
    qvL2->addLayout(dirRow);

    // Queue items row — all 7 moves for n=3
    struct QMv { QString disk; QString from; QString to; QString bg; };
    QMv moves7[] = {
                    {"Disk 1","A","C","#ff6b6b"},
                    {"Disk 2","A","B","#ff922b"},
                    {"Disk 1","C","B","#ff6b6b"},
                    {"Disk 3","A","C","#845ef7"},
                    {"Disk 1","B","A","#ff6b6b"},
                    {"Disk 2","B","C","#ff922b"},
                    {"Disk 1","A","C","#ff6b6b"},
                    };

    QHBoxLayout *qRow = new QHBoxLayout;
    qRow->setSpacing(4);

    // FRONT bracket
    QLabel *frontBracket = new QLabel("[");
    frontBracket->setStyleSheet(
        "color:#fb7185; font-size:22px; font-weight:bold; background:transparent;");
    qRow->addWidget(frontBracket);

    for(int i = 0; i < 7; i++){
        QWidget *cell = new QWidget;
        cell->setFixedSize(74, 54);
        // color border based on disk
        QString borderCol = QString(moves7[i].bg).replace("#","rgba(").append(",0.5)");
        cell->setStyleSheet(QString(
                                "background: rgba(78,168,222,0.07);"
                                "border: 1px solid %1;"
                                "border-radius: 7px;").arg(moves7[i].bg));
        QVBoxLayout *cl = new QVBoxLayout(cell);
        cl->setContentsMargins(2,3,2,3);
        cl->setSpacing(2);

        QLabel *step = new QLabel(QString("Step %1").arg(i+1));
        step->setAlignment(Qt::AlignCenter);
        step->setStyleSheet(
            "color:#585b70; font-size:8px;"
            "font-family:'Courier New'; background:transparent;");

        QLabel *dskLbl = new QLabel(moves7[i].disk);
        dskLbl->setAlignment(Qt::AlignCenter);
        dskLbl->setStyleSheet(QString(
                                  "color:%1; font-size:10px; font-weight:bold;"
                                  "font-family:'Courier New'; background:transparent;").arg(moves7[i].bg));

        QLabel *mvLbl = new QLabel(
            QString("%1  →  %2").arg(moves7[i].from).arg(moves7[i].to));
        mvLbl->setAlignment(Qt::AlignCenter);
        mvLbl->setStyleSheet(
            "color:#CDD6F4; font-size:11px; font-weight:bold;"
            "font-family:'Courier New'; background:transparent;");

        cl->addWidget(step);
        cl->addWidget(dskLbl);
        cl->addWidget(mvLbl);
        qRow->addWidget(cell);

        if(i < 6){
            QLabel *arr = new QLabel("›");
            arr->setStyleSheet(
                "color:#313244; font-size:16px; background:transparent;");
            qRow->addWidget(arr);
        }
    }

    // BACK bracket
    QLabel *backBracket = new QLabel("]");
    backBracket->setStyleSheet(
        "color:#a6e3a1; font-size:22px; font-weight:bold; background:transparent;");
    qRow->addWidget(backBracket);
    qRow->addStretch();
    qvL2->addLayout(qRow);

    // Explanation row
    QHBoxLayout *explRow = new QHBoxLayout;
    QLabel *explFront = new QLabel("↑ Step 1 executes first (FIFO)");
    explFront->setStyleSheet(
        "color:#fb7185; font-size:10px; font-family:'Courier New'; background:transparent;");
    QLabel *explBack = new QLabel("Step 7 executes last ↑");
    explBack->setStyleSheet(
        "color:#a6e3a1; font-size:10px; font-family:'Courier New'; background:transparent;");
    explRow->addWidget(explFront);
    explRow->addStretch();
    explRow->addWidget(explBack);
    qvL2->addLayout(explRow);

    qlL->addWidget(qViz);

    qlL->addWidget(mkCode(
        "// Move struct stored in Queue\n"
        "struct Move {\n"
        "    std::string from, to;\n"
        "    int diskSize;\n"
        "};\n\n"
        "std::queue<Move> solutionQueue;\n\n"
        "// Recursion fills the Queue\n"
        "void generateSolution(int n,\n"
        "    string src, string aux, string dst) {\n"
        "    if (n == 0) return;\n"
        "    generateSolution(n-1, src, dst, aux);\n"
        "    solutionQueue.push({src, dst, n}); // enqueue\n"
        "    generateSolution(n-1, aux, src, dst);\n"
        "}\n\n"
        "// Timer dequeues one move per tick\n"
        "void onAutoSolveStep() {\n"
        "    Move m = solutionQueue.front(); // peek\n"
        "    solutionQueue.pop();            // dequeue\n"
        "    game.moveDisk(m.from, m.to);   // execute\n"
        "}"
        ));

    qlL->addStretch();
    pages->addWidget(saQueue);

    // ═══════════════════════════════════════════════════════
    // PAGE 3 — TREE
    // ═══════════════════════════════════════════════════════
    QWidget *pgTree = new QWidget;
    pgTree->setStyleSheet("background:#12121F;");
    QScrollArea *saTree = new QScrollArea;
    saTree->setWidget(pgTree);
    saTree->setWidgetResizable(true);
    saTree->setStyleSheet("QScrollArea{border:none;background:#12121F;}"
                          "QScrollBar:vertical{width:4px;background:#1E1E2E;}"
                          "QScrollBar::handle:vertical{background:#313244;border-radius:2px;}");

    QVBoxLayout *tlL = new QVBoxLayout(pgTree);
    tlL->setContentsMargins(24,20,24,20);
    tlL->setSpacing(14);

    tlL->addWidget(mkSection("🌳","Recursion Tree","#fcc419","Binary Tree — Divide & Conquer"));

    tlL->addWidget(mkLabel(
        "<span style='color:#CDD6F4;font-size:13px;'>"
        "The recursive algorithm naturally forms a <b>binary tree</b>. Each node is a call to "
        "<span style='color:#fcc419;font-family:Courier New;'>Hanoi(n, src, aux, dst)</span>. "
        "It splits into two children of size n−1, with the root action (moving the largest disk) in between. "
        "This is an <b>in-order traversal</b> of the recursion tree."
        "</span>",
        "background:transparent;"));

    // Tree diagram using QLabel grid
    QWidget *treeViz = new QWidget;
    treeViz->setStyleSheet("background:#1E1E2E;border:1px solid #313244;border-radius:12px;");
    treeViz->setFixedHeight(200);

    QVBoxLayout *tvLayout = new QVBoxLayout(treeViz);
    tvLayout->setContentsMargins(16,12,16,12);
    tvLayout->setSpacing(4);

    auto mkNode=[](const QString &txt, const QString &col, int w=90)->QLabel*{
        QLabel *n=new QLabel(txt);
        n->setFixedSize(w,34);
        n->setAlignment(Qt::AlignCenter);
        n->setStyleSheet(QString(
                             "background:rgba(255,255,255,0.05);border:1px solid %1;"
                             "border-radius:17px;color:%1;font-size:10px;"
                             "font-family:'Courier New';font-weight:bold;").arg(col));
        return n;
    };
    auto mkArrow=[](const QString &txt="#585b70")->QLabel*{
        QLabel *a=new QLabel("↙          ↘");
        a->setAlignment(Qt::AlignCenter);
        a->setStyleSheet(QString("color:%1;font-size:12px;background:transparent;").arg(txt));
        return a;
    };
    auto mkMoveNode=[](const QString &txt)->QLabel*{
        QLabel *n=new QLabel(txt);
        n->setFixedSize(90,28);
        n->setAlignment(Qt::AlignCenter);
        n->setStyleSheet(
            "background:rgba(252,196,73,0.18);border:1px solid #fcc419;"
            "border-radius:6px;color:#fcc419;font-size:9px;"
            "font-family:'Courier New';font-weight:bold;");
        return n;
    };

    // Row 1 — root
    QHBoxLayout *r1=new QHBoxLayout; r1->addStretch();
    r1->addWidget(mkNode("Hanoi(3)","#fcc419",100)); r1->addStretch();
    tvLayout->addLayout(r1);

    // Arrows
    tvLayout->addWidget(mkArrow("#fcc419"));

    // Row 2
    QHBoxLayout *r2=new QHBoxLayout; r2->setSpacing(8);
    r2->addWidget(mkNode("Hanoi(2)","#a0a0c0"));
    r2->addStretch();
    r2->addWidget(mkMoveNode("MOVE disk 3\nA → C"));
    r2->addStretch();
    r2->addWidget(mkNode("Hanoi(2)","#a0a0c0"));
    tvLayout->addLayout(r2);

    tvLayout->addWidget(mkArrow());

    // Row 3
    QHBoxLayout *r3=new QHBoxLayout; r3->setSpacing(4);
    r3->addWidget(mkNode("H(1)","#585b70",70));
    r3->addWidget(mkMoveNode("disk 2\nA→B"));
    r3->addWidget(mkNode("H(1)","#585b70",70));
    r3->addStretch();
    r3->addWidget(mkNode("H(1)","#585b70",70));
    r3->addWidget(mkMoveNode("disk 2\nB→C"));
    r3->addWidget(mkNode("H(1)","#585b70",70));
    tvLayout->addLayout(r3);

    QLabel *complexity = new QLabel("Depth = n  |  Total nodes = 2ⁿ−1  |  Time: O(2ⁿ)  |  Space: O(n)");
    complexity->setAlignment(Qt::AlignCenter);
    complexity->setStyleSheet("color:#585b70;font-size:10px;font-family:'Courier New';background:transparent;margin-top:4px;");
    tvLayout->addWidget(complexity);

    tlL->addWidget(treeViz);

    tlL->addWidget(mkCode(
        "// Recursive algorithm — forms a binary tree\n"
        "void Hanoi(int n, string src,\n"
        "           string aux, string dst) {\n"
        "    if (n == 0) return;  // base case (leaf)\n\n"
        "    // LEFT subtree: move n-1 disks to aux\n"
        "    Hanoi(n-1, src, dst, aux);\n\n"
        "    // ROOT action: move largest disk\n"
        "    moveDisk(src, dst);   // enqueue to Queue\n\n"
        "    // RIGHT subtree: move n-1 disks to dst\n"
        "    Hanoi(n-1, aux, src, dst);\n"
        "}\n\n"
        "// Minimum moves = 2^n - 1\n"
        "// For 3 disks: 2^3 - 1 = 7 moves\n"
        "// For 8 disks: 2^8 - 1 = 255 moves"
        ));

    tlL->addStretch();
    pages->addWidget(saTree);

    // ═══════════════════════════════════════════════════════
    // PAGE 4 — ABOUT GAME
    // ═══════════════════════════════════════════════════════
    QWidget *pgAbout = new QWidget;
    pgAbout->setStyleSheet("background:#12121F;");
    QScrollArea *saAbout = new QScrollArea;
    saAbout->setWidget(pgAbout);
    saAbout->setWidgetResizable(true);
    saAbout->setStyleSheet("QScrollArea{border:none;background:#12121F;}"
                           "QScrollBar:vertical{width:4px;background:#1E1E2E;}"
                           "QScrollBar::handle:vertical{background:#313244;border-radius:2px;}");

    QVBoxLayout *abL = new QVBoxLayout(pgAbout);
    abL->setContentsMargins(24,20,24,20);
    abL->setSpacing(14);

    QLabel *gameTitle = new QLabel("🗼  Tower of Hanoi");
    gameTitle->setStyleSheet("font-size:22px;font-weight:bold;color:#CDD6F4;background:transparent;");
    abL->addWidget(gameTitle);

    abL->addWidget(mkLabel(
        "<span style='color:#CDD6F4;font-size:13px;line-height:1.8;'>"
        "Tower of Hanoi is a classic mathematical puzzle invented in <b style='color:#fcc419;'>1883</b> by "
        "French mathematician <b style='color:#fcc419;'>Édouard Lucas</b>. "
        "It consists of three rods and a number of disks of different sizes. "
        "The puzzle starts with disks stacked on rod A in decreasing size order."
        "</span>",
        "background:transparent;"));

    // Rules
    QWidget *rulesBox = new QWidget;
    rulesBox->setStyleSheet("background:#1E1E2E;border:1px solid #313244;border-radius:12px;padding:4px;");
    QVBoxLayout *rbL = new QVBoxLayout(rulesBox);
    rbL->setContentsMargins(16,12,16,12);
    rbL->setSpacing(10);
    QLabel *rulesTitle = new QLabel("Game Rules");
    rulesTitle->setStyleSheet("font-size:13px;font-weight:bold;color:#a6e3a1;background:transparent;");
    rbL->addWidget(rulesTitle);
    struct Rule { QString icon; QString txt; };
    Rule rules[] = {
                    {"1️⃣","Only ONE disk can be moved at a time — the top disk of any tower"},
                    {"2️⃣","A LARGER disk can NEVER be placed on top of a smaller disk"},
                    {"3️⃣","Goal: move ALL disks from Tower A (Source) → Tower C (Destination)"},
                    {"⚡","Minimum moves required = 2ⁿ − 1   (e.g. 3 disks = 7 moves, 8 disks = 255 moves)"},
                    };
    for(auto &r:rules){
        QHBoxLayout *rl=new QHBoxLayout;
        QLabel *ic=new QLabel(r.icon); ic->setFixedWidth(28);
        ic->setStyleSheet("background:transparent;font-size:14px;");
        QLabel *tx=new QLabel(r.txt);
        tx->setStyleSheet("color:#CDD6F4;font-size:12px;font-family:'Courier New';background:transparent;");
        tx->setWordWrap(true);
        rl->addWidget(ic); rl->addWidget(tx);
        rbL->addLayout(rl);
    }
    abL->addWidget(rulesBox);

    // Controls
    QWidget *ctrlBox = new QWidget;
    ctrlBox->setStyleSheet("background:#1E1E2E;border:1px solid #313244;border-radius:12px;");
    QVBoxLayout *ctrlL = new QVBoxLayout(ctrlBox);
    ctrlL->setContentsMargins(16,12,16,12); ctrlL->setSpacing(8);
    QLabel *ctrlTitle = new QLabel("How to Play");
    ctrlTitle->setStyleSheet("font-size:13px;font-weight:bold;color:#89b4fa;background:transparent;");
    ctrlL->addWidget(ctrlTitle);
    struct Ctrl { QString key; QString desc; };
    Ctrl ctrls[] = {
                    {"Click tower","Select the top disk (highlighted in orange)"},
                    {"Click again","Place disk on chosen tower (or cancel if same tower)"},
                    {"Drag & Drop","Click and drag a disk directly to another tower"},
                    {"↩ Undo","Reverse your last move using the Undo Stack"},
                    {"⚡ Auto Solve","Watch the algorithm solve it step by step using Queue"},
                    };
    for(auto &c:ctrls){
        QHBoxLayout *cl=new QHBoxLayout;
        QLabel *kl=new QLabel(c.key);
        kl->setFixedWidth(110);
        kl->setStyleSheet(
            "color:#89b4fa;font-size:11px;font-family:'Courier New';"
            "font-weight:bold;background:transparent;");
        QLabel *dl=new QLabel("— "+c.desc);
        dl->setStyleSheet("color:#6C7086;font-size:11px;font-family:'Courier New';background:transparent;");
        dl->setWordWrap(true);
        cl->addWidget(kl); cl->addWidget(dl);
        ctrlL->addLayout(cl);
    }
    abL->addWidget(ctrlBox);

    // Viva summary
    QWidget *vivaBox = new QWidget;
    vivaBox->setStyleSheet("background:#1E1E2E;border:1px solid #313244;border-radius:12px;");
    QVBoxLayout *vivaL = new QVBoxLayout(vivaBox);
    vivaL->setContentsMargins(16,12,16,12); vivaL->setSpacing(6);
    QLabel *vivaTitle = new QLabel("Quick Viva Answers");
    vivaTitle->setStyleSheet("font-size:13px;font-weight:bold;color:#fb7185;background:transparent;");
    vivaL->addWidget(vivaTitle);
    struct QA { QString q; QString a; };
    QA qas[] = {
                {"Why Stack?",    "Towers follow LIFO — only the top disk can be moved"},
                {"Why Queue?",    "Auto-solve needs FIFO order — first move generated must execute first"},
                {"Time complexity?", "O(2ⁿ) — each extra disk doubles the number of moves"},
                {"Space complexity?","O(n) — recursion call stack depth equals number of disks"},
                {"How does Undo work?","Every move is pushed to an undo Stack; Undo pops and reverses it"},
                };
    for(auto &qa:qas){
        QLabel *ql=new QLabel(
            QString("<span style='color:#fb7185;font-family:Courier New;font-size:11px;'>Q: %1</span>"
                    "<br><span style='color:#6C7086;font-family:Courier New;font-size:11px;'>A: %2</span>")
                .arg(qa.q).arg(qa.a));
        ql->setStyleSheet("background:rgba(255,255,255,0.02);border-radius:6px;padding:4px 6px;");
        ql->setTextFormat(Qt::RichText);
        ql->setWordWrap(true);
        vivaL->addWidget(ql);
    }
    abL->addWidget(vivaBox);
    abL->addStretch();
    pages->addWidget(saAbout);

    // ── Tab switching ─────────────────────────────────────────────────────────
    tabBtns[0]->setChecked(true);
    for(int i=0;i<4;i++){
        QPushButton *btn = tabBtns[i];
        connect(btn, &QPushButton::clicked, this, [=](){
            for(auto b:tabBtns) b->setChecked(false);
            btn->setChecked(true);
            pages->setCurrentIndex(i);
        });
    }

    // ── Close button ──────────────────────────────────────────────────────────
    QWidget *footer = new QWidget;
    footer->setFixedHeight(52);
    footer->setStyleSheet("background:#1E1E2E;border-top:1px solid #313244;");
    QHBoxLayout *footL = new QHBoxLayout(footer);
    footL->setContentsMargins(16,8,16,8);
    footL->addStretch();
    QPushButton *closeBtn = new QPushButton("Close");
    closeBtn->setFixedSize(100,34);
    closeBtn->setStyleSheet(
        "QPushButton{background:#313244;color:#CDD6F4;font-weight:bold;"
        "border-radius:7px;font-size:13px;}"
        "QPushButton:pressed{background:#45475A;}");
    connect(closeBtn, &QPushButton::clicked, &dlg, &QDialog::accept);
    footL->addWidget(closeBtn);
    mainL->addWidget(footer);

    dlg.exec();
}
