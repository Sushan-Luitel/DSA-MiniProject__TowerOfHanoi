#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTimer>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QListWidget>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <QDialog>
#include "game.h"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

private slots:
    void onAutoSolveClicked();
    void onUndoClicked();
    void onResetClicked();
    void onAutoSolveStep();
    void onTimerTick();
    void onAnimStep();
    void onAboutClicked();

private:
    Game game;
    int  numDisks;
    int  selectedTower;

    // Drag state
    bool  dragging;
    int   dragFromTower;
    QGraphicsRectItem *dragGhost;

    // Animation state
    bool  animating;
    int   animFrom, animTo;
    int   animDiskSz;
    float animProgress;   // 0.0 → 1.0
    QGraphicsRectItem *animItem;
    QTimer *animTimer;
    // arc keypoints
    float animSX, animSY, animEX, animEY, animArcY;

    QGraphicsScene *scene;
    QGraphicsView  *view;
    QPushButton    *btnAutoSolve;
    QPushButton    *btnUndo;
    QPushButton    *btnReset;
    QPushButton    *btnAbout;
    QComboBox      *comboDiskCount;
    QListWidget    *moveLogList;
    QLabel         *labelMoveCount;
    QLabel         *labelTimer;
    QLabel         *labelStatus;
    QLabel         *labelHelp;

    QTimer *autoSolveTimer;
    QTimer *clockTimer;
    int  elapsedSeconds;
    bool gameRunning;

    void handleMousePress(QPointF sp);
    void handleMouseMove(QPointF sp);
    void handleMouseRelease(QPointF sp);
    void handleTowerClick(int idx);
    void doMove(int from, int to);        // executes move + starts animation
    void finishMove(int from, int to);    // called after animation done
    void showWinDialog();
    void showAboutDialog();
    void checkWin();

    void redraw();
    void drawTowers();
    void drawDisks();
    void drawDisksExcept(int skipTower, int skipTopSz);
    void updateMoveLog();
    void updateStatus(const QString &msg);

    int   towerAtX(int x);
    int   towerX(int idx);
    int   baseY();
    int   diskW(int sz);
    QColor diskColor(int sz);

    static const int SCENE_W    = 750;
    static const int SCENE_H    = 400;
    static const int ROD_H      = 270;
    static const int BASE_H     = 14;
    static const int DISK_H     = 26;
    static const int MAX_DISK_W = 185;
    static const int MIN_DISK_W = 38;
};

#endif
