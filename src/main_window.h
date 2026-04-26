#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include <QMainWindow>
#include <QStackedWidget>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QListWidget>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QTextEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>
#include <memory>
#include <vector>
#include <functional>

#include "database.h"
#include "quiz_engine.h"
#include "file_parser.h"
#include "models.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow() = default;

private slots:
    // Navigation
    void onHomeClicked();
    void onNewClicked();
    void onImportClicked();
    void onPlayClicked();
    void onDeleteClicked();
    void onSettingsClicked();

    // Home page
    void onNewQuizClicked();
    void onImportFileClicked();
    void onPlayQuizClicked();
    void onDeleteSetClicked();

    // New Quiz page
    void onAddAnswerClicked();
    void onAddQuestionClicked();
    void onSaveNewQuizClicked();

    // Play page
    void onStartQuizClicked();
    void onAnswerClicked();
    void onNextQuestionClicked();

    // Delete page
    void onRefreshDeleteListClicked();
    void onConfirmDeleteClicked();

    // Import page
    void onBrowseFileClicked();
    void onImportFileDataClicked();

    // Settings page
    void onSaveSettingsClicked();

private:
    // ========== UI SETUP ==========
    void setupUI();
    void setupHeaderBar();
    void setupHomePage();
    void setupNewQuizPage();
    void setupImportPage();
    void setupPlayPage();
    void setupDeletePage();
    void setupSettingsPage();
    void applyStyles();

    // ========== PAGES ==========
    QWidget* createHomePage();
    QWidget* createNewQuizPage();
    QWidget* createImportPage();
    QWidget* createPlayPage();
    QWidget* createDeletePage();
    QWidget* createSettingsPage();

    // ========== HELPERS ==========
    void refreshQuizSets();
    void refreshHomePage();
    void refreshDeleteList();
    void updateQuizDisplay();
    void showQuizResult();
    void showErrorDialog(const QString& title, const QString& message);
    void showInfoDialog(const QString& title, const QString& message);

    // ========== DATA ==========
    std::unique_ptr<Database> db;
    QuizEngine quizEngine;
    AppSettings appSettings;

    std::vector<QuizSet> allQuizSets;
    TXTParser::ParseResult lastParseResult;
    int selectedQuizSetId;
    QString selectedFilePath;

    // ========== WIDGETS - HOME ==========
    QLabel* homeStatsLabel;

    // ========== WIDGETS - NEW QUIZ ==========
    QLineEdit* newQuizNameInput;
    QTextEdit* questionTextInput;
    QLineEdit* answerInput;
    QListWidget* answersList;
    QListWidget* questionsPreviewList;
    QLabel* questionCountLabel;
    std::vector<std::pair<QLineEdit*, QCheckBox*>> answerInputs;

    // ========== WIDGETS - PLAY ==========
    QListWidget* quizSetsListWidget;
    QLabel* questionLabel;
    QLabel* progressLabel;
    QLabel* scoreLabel;
    QProgressBar* questionProgressBar;
    QVBoxLayout* answersButtonsLayout;
    std::vector<QPushButton*> answerButtons;
    QPushButton* nextQuestionButton;

    // ========== WIDGETS - DELETE ==========
    QListWidget* deleteSetsListWidget;

    // ========== WIDGETS - IMPORT ==========
    QLabel* selectedFileLabel;
    QLabel* parseResultLabel;
    QLineEdit* importQuizNameInput;
    QListWidget* importedQuestionsListWidget;

    // ========== WIDGETS - SETTINGS ==========
    QCheckBox* darkModeCheckbox;
    QComboBox* languageComboBox;
    QSpinBox* fontSizeSpinBox;
    QSpinBox* answerTimeSpinBox;
    QCheckBox* allowRepeatCheckbox;
    QCheckBox* soundEnabledCheckbox;

    // ========== CENTRAL WIDGET ==========
    QStackedWidget* stackedWidget;
};

#endif // MAIN_WINDOW_H
