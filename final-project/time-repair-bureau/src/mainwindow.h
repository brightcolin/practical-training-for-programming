#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class GameWidget;
class EditorWidget;
class QStackedWidget;
class QWidget;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    QWidget *createStartPage();
    QWidget *createLevelSelectPage();
    QWidget *createStoryPage();
    QWidget *createHelpPage();
    QWidget *createSettingsPage();
    QWidget *createAboutPage();
    QWidget *createCodexPage();
    QWidget *createReverseSelectPage();
    QWidget *createDialoguePage();
    QWidget *createMissionBriefingPage();
    QWidget *createResultPage(const QString &title, const QString &subtitle, bool victory);
    QWidget *createFailureAnnouncePage();
    void refreshResultPage(bool victory);
    void continueMainStory();
    void startNewStory();
    void showOpeningPrologue();
    void showPrologueThenStart();
    void showRecruitBriefingThenStart();
    void startLevel(int index);
    void launchLevel(int index);
    void launchReverseMode(int scenarioIndex = 0);
    void showMissionBriefing(int index);
    void showPrologueDialogue();
    void showIntroDialogue(int index);
    void showOutroDialogue(bool victory);
    void showStartPage();
    void showLevelSelectPage();
    void showStoryPage();
    void showHelpPage();
    void showSettingsPage();
    void showAboutPage();
    void showCodexPage();
    void showReverseSelectPage();
    void showEditorPage();
    void playCustomLevel(const QString &path);
    void showVictoryPage();
    void showFailurePage();
    void startNextLevel();

    QStackedWidget *m_stack = nullptr;
    GameWidget *m_game = nullptr;
    EditorWidget *m_editor = nullptr;
    QWidget *m_startPage = nullptr;
    QWidget *m_openingPage = nullptr;
    QWidget *m_levelSelectPage = nullptr;
    QWidget *m_storyPage = nullptr;
    QWidget *m_helpPage = nullptr;
    QWidget *m_settingsPage = nullptr;
    QWidget *m_aboutPage = nullptr;
    QWidget *m_codexPage = nullptr;
    QWidget *m_reverseSelectPage = nullptr;
    QWidget *m_dialoguePage = nullptr;
    QWidget *m_missionBriefingPage = nullptr;
    QWidget *m_failureAnnouncePage = nullptr;
    QWidget *m_victoryPage = nullptr;
    QWidget *m_failurePage = nullptr;
    int m_pendingLevelIndex = 0;
    bool m_acceptanceMode = false;
    bool m_archiveReturnMode = false;
    bool m_settingsReturnToGame = false;
    bool m_reverseSession = false;
};

#endif
