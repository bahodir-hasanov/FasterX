#include "main_window.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QLineEdit>
#include <QTextEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QProgressBar>
#include <QCheckBox>
#include <QSpinBox>
#include <QComboBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QStandardPaths>
#include <QApplication>
#include <QStyle>
#include <iostream>

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent), selectedQuizSetId(-1) {
    
    setWindowTitle("Quiz Master Pro");
    setGeometry(100, 100, 900, 700);
    setMinimumSize(800, 600);

    try {
        db = std::make_unique<Database>();
    } catch (const std::exception& e) {
        showErrorDialog("Error", QString::fromStdString(e.what()));
    }

    setupUI();
    applyStyles();
    show();
}

void MainWindow::setupUI() {
    stackedWidget = new QStackedWidget(this);
    setCentralWidget(stackedWidget);

    stackedWidget->addWidget(createHomePage());      // 0
    stackedWidget->addWidget(createNewQuizPage());   // 1
    stackedWidget->addWidget(createImportPage());    // 2
    stackedWidget->addWidget(createPlayPage());      // 3
    stackedWidget->addWidget(createDeletePage());    // 4
    stackedWidget->addWidget(createSettingsPage());  // 5

    stackedWidget->setCurrentIndex(0);
}

QWidget* MainWindow::createHomePage() {
    QWidget* page = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(page);

    QLabel* titleLabel = new QLabel("📚 QUIZ MASTER PRO");
    titleLabel->setStyleSheet("font-size: 24px; font-weight: bold;");
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);

    QLabel* subtitleLabel = new QLabel("Bilimingizni sinovdan o'tkazing");
    subtitleLabel->setStyleSheet("font-size: 14px; color: #666;");
    subtitleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(subtitleLabel);

    QGridLayout* gridLayout = new QGridLayout();
    
    QPushButton* newQuizBtn = new QPushButton("✨ YANGI QUIZ");
    newQuizBtn->setMinimumHeight(100);
    connect(newQuizBtn, &QPushButton::clicked, this, &MainWindow::onNewClicked);
    gridLayout->addWidget(newQuizBtn, 0, 0);

    QPushButton* importBtn = new QPushButton("📂 YUKLASH");
    importBtn->setMinimumHeight(100);
    connect(importBtn, &QPushButton::clicked, this, &MainWindow::onImportClicked);
    gridLayout->addWidget(importBtn, 0, 1);

    QPushButton* playBtn = new QPushButton("🎮 O'YNA");
    playBtn->setMinimumHeight(100);
    connect(playBtn, &QPushButton::clicked, this, &MainWindow::onPlayClicked);
    gridLayout->addWidget(playBtn, 1, 0);

    QPushButton* deleteBtn = new QPushButton("🗑️ O'CHIRISH");
    deleteBtn->setMinimumHeight(100);
    connect(deleteBtn, &QPushButton::clicked, this, &MainWindow::onDeleteClicked);
    gridLayout->addWidget(deleteBtn, 1, 1);

    layout->addLayout(gridLayout);

    homeStatsLabel = new QLabel();
    homeStatsLabel->setAlignment(Qt::AlignTop);
    layout->addWidget(homeStatsLabel);

    layout->addStretch();
    return page;
}

QWidget* MainWindow::createNewQuizPage() {
    QWidget* page = new QWidget();
    QVBoxLayout* mainLayout = new QVBoxLayout(page);

    QLabel* titleLabel = new QLabel("✨ YANGI QUIZ SET YARATISH");
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold;");
    mainLayout->addWidget(titleLabel);

    mainLayout->addWidget(new QLabel("QUIZ NOMI:"));
    newQuizNameInput = new QLineEdit();
    mainLayout->addWidget(newQuizNameInput);

    mainLayout->addWidget(new QLabel("SAVOL MATNI:"));
    questionTextInput = new QTextEdit();
    questionTextInput->setMaximumHeight(100);
    mainLayout->addWidget(questionTextInput);

    mainLayout->addWidget(new QLabel("JAVOBLAR:"));
    answerInput = new QLineEdit();
    mainLayout->addWidget(answerInput);

    answersList = new QListWidget();
    mainLayout->addWidget(answersList);

    QPushButton* addAnswerBtn = new QPushButton("+ JAVOB QO'SHISH");
    connect(addAnswerBtn, &QPushButton::clicked, this, &MainWindow::onAddAnswerClicked);
    mainLayout->addWidget(addAnswerBtn);

    mainLayout->addWidget(new QLabel("SAVOLLAR:"));
    questionsPreviewList = new QListWidget();
    mainLayout->addWidget(questionsPreviewList);
    
    questionCountLabel = new QLabel("Jami: 0 ta savol");
    mainLayout->addWidget(questionCountLabel);

    QPushButton* addQuestionBtn = new QPushButton("➕ SAVOL QO'SHISH");
    connect(addQuestionBtn, &QPushButton::clicked, this, &MainWindow::onAddQuestionClicked);
    mainLayout->addWidget(addQuestionBtn);

    QPushButton* saveBtn = new QPushButton("💾 SAQLASH");
    connect(saveBtn, &QPushButton::clicked, this, &MainWindow::onSaveNewQuizClicked);
    mainLayout->addWidget(saveBtn);

    mainLayout->addStretch();
    return page;
}

QWidget* MainWindow::createImportPage() {
    QWidget* page = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(page);

    QLabel* titleLabel = new QLabel("📂 TXT FAYLDAN YUKLASH");
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold;");
    layout->addWidget(titleLabel);

    layout->addWidget(new QLabel("Faylni tanlang:"));
    selectedFileLabel = new QLabel("Hech narsa tanlanmadi");
    layout->addWidget(selectedFileLabel);

    QPushButton* browseBtn = new QPushButton("📁 TANLASH");
    connect(browseBtn, &QPushButton::clicked, this, &MainWindow::onBrowseFileClicked);
    layout->addWidget(browseBtn);

    layout->addWidget(new QLabel("Parse natijalari:"));
    parseResultLabel = new QLabel();
    layout->addWidget(parseResultLabel);

    importedQuestionsListWidget = new QListWidget();
    layout->addWidget(importedQuestionsListWidget);

    layout->addWidget(new QLabel("QUIZ NOMI:"));
    importQuizNameInput = new QLineEdit();
    layout->addWidget(importQuizNameInput);

    QPushButton* importBtn = new QPushButton("📥 IMPORT QILISH");
    connect(importBtn, &QPushButton::clicked, this, &MainWindow::onImportFileDataClicked);
    layout->addWidget(importBtn);

    layout->addStretch();
    return page;
}

QWidget* MainWindow::createPlayPage() {
    QWidget* page = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(page);

    QLabel* titleLabel = new QLabel("🎮 QUIZ O'YNA");
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold;");
    layout->addWidget(titleLabel);

    layout->addWidget(new QLabel("QUIZ SET TANLANG:"));
    quizSetsListWidget = new QListWidget();
    layout->addWidget(quizSetsListWidget);

    QPushButton* startBtn = new QPushButton("🎬 QUIZNI BOSHLASH");
    connect(startBtn, &QPushButton::clicked, this, &MainWindow::onStartQuizClicked);
    layout->addWidget(startBtn);

    layout->addSpacing(20);

    questionLabel = new QLabel();
    layout->addWidget(questionLabel);

    progressLabel = new QLabel();
    layout->addWidget(progressLabel);

    questionProgressBar = new QProgressBar();
    layout->addWidget(questionProgressBar);

    answersButtonsLayout = new QVBoxLayout();
    layout->addLayout(answersButtonsLayout);

    scoreLabel = new QLabel("BALL: 0/0");
    layout->addWidget(scoreLabel);

    nextQuestionButton = new QPushButton("⏭️ KEYINGI SAVOL");
    connect(nextQuestionButton, &QPushButton::clicked, this, &MainWindow::onNextQuestionClicked);
    layout->addWidget(nextQuestionButton);

    layout->addStretch();
    return page;
}

QWidget* MainWindow::createDeletePage() {
    QWidget* page = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(page);

    QLabel* titleLabel = new QLabel("🗑️ QUIZ SETNI O'CHIRISH");
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold;");
    layout->addWidget(titleLabel);

    layout->addWidget(new QLabel("O'CHIRISH UCHUN SET TANLANG:"));
    deleteSetsListWidget = new QListWidget();
    layout->addWidget(deleteSetsListWidget);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    
    QPushButton* refreshBtn = new QPushButton("🔄 YANGILASH");
    connect(refreshBtn, &QPushButton::clicked, this, &MainWindow::onRefreshDeleteListClicked);
    btnLayout->addWidget(refreshBtn);

    QPushButton* deleteBtn = new QPushButton("🗑️ O'CHIRISH");
    connect(deleteBtn, &QPushButton::clicked, this, &MainWindow::onConfirmDeleteClicked);
    btnLayout->addWidget(deleteBtn);

    layout->addLayout(btnLayout);
    layout->addStretch();
    return page;
}

QWidget* MainWindow::createSettingsPage() {
    QWidget* page = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(page);

    QLabel* titleLabel = new QLabel("⚙️ SOZLAMALAR");
    titleLabel->setStyleSheet("font-size: 18px; font-weight: bold;");
    layout->addWidget(titleLabel);

    layout->addWidget(new QLabel("🎨 INTERFEYS:"));
    
    darkModeCheckbox = new QCheckBox("QORONG'I REJIM");
    layout->addWidget(darkModeCheckbox);

    layout->addWidget(new QLabel("TIL:"));
    languageComboBox = new QComboBox();
    languageComboBox->addItems({"O'zbek", "English", "Русский"});
    layout->addWidget(languageComboBox);

    layout->addWidget(new QLabel("SHRIFT O'LCHAMI:"));
    fontSizeSpinBox = new QSpinBox();
    fontSizeSpinBox->setRange(8, 20);
    fontSizeSpinBox->setValue(14);
    layout->addWidget(fontSizeSpinBox);

    layout->addWidget(new QLabel("🎮 QUIZ SOZLAMALARI:"));
    
    layout->addWidget(new QLabel("JAVOB VAQTI (sekund):"));
    answerTimeSpinBox = new QSpinBox();
    answerTimeSpinBox->setRange(10, 120);
    answerTimeSpinBox->setValue(30);
    layout->addWidget(answerTimeSpinBox);

    allowRepeatCheckbox = new QCheckBox("TAKRORLANISHGA RUHSAT");
    layout->addWidget(allowRepeatCheckbox);

    soundEnabledCheckbox = new QCheckBox("TOVUSH");
    layout->addWidget(soundEnabledCheckbox);

    layout->addStretch();

    QPushButton* saveBtn = new QPushButton("💾 SAQLASH");
    connect(saveBtn, &QPushButton::clicked, this, &MainWindow::onSaveSettingsClicked);
    layout->addWidget(saveBtn);

    return page;
}

void MainWindow::applyStyles() {
    QString styleSheet = R"(
        QMainWindow { background-color: #f5f5f5; }
        QPushButton { background-color: #667eea; color: white; border: none; 
                     border-radius: 8px; padding: 10px 20px; font-weight: bold; }
        QPushButton:hover { background-color: #764ba2; }
        QLineEdit, QTextEdit { border: 2px solid #e0e0e0; border-radius: 8px; 
                              padding: 8px; background: white; }
        QLineEdit:focus, QTextEdit:focus { border: 2px solid #667eea; }
    )";
    qApp->setStyleSheet(styleSheet);
}

void MainWindow::onHomeClicked() { stackedWidget->setCurrentIndex(0); }
void MainWindow::onNewClicked() { stackedWidget->setCurrentIndex(1); }
void MainWindow::onImportClicked() { stackedWidget->setCurrentIndex(2); }
void MainWindow::onPlayClicked() { stackedWidget->setCurrentIndex(3); refreshQuizSets(); }
void MainWindow::onDeleteClicked() { stackedWidget->setCurrentIndex(4); refreshDeleteList(); }
void MainWindow::onSettingsClicked() { stackedWidget->setCurrentIndex(5); }

void MainWindow::onNewQuizClicked() {}
void MainWindow::onImportFileClicked() {}
void MainWindow::onPlayQuizClicked() {}
void MainWindow::onDeleteSetClicked() {}

void MainWindow::onAddAnswerClicked() {}
void MainWindow::onAddQuestionClicked() {}
void MainWindow::onSaveNewQuizClicked() {}

void MainWindow::onStartQuizClicked() {}
void MainWindow::onAnswerClicked() {}
void MainWindow::onNextQuestionClicked() {}

void MainWindow::onRefreshDeleteListClicked() { refreshDeleteList(); }
void MainWindow::onConfirmDeleteClicked() {}

void MainWindow::onBrowseFileClicked() {
    selectedFilePath = QFileDialog::getOpenFileName(this, "Faylni tanlang", "", "Text Files (*.txt)");
    if (!selectedFilePath.isEmpty()) {
        selectedFileLabel->setText(selectedFilePath);
    }
}
void MainWindow::onImportFileDataClicked() {}
void MainWindow::onSaveSettingsClicked() {}

void MainWindow::refreshQuizSets() {
    if (!db) return;
    allQuizSets = db->getAllQuizSets();
    quizSetsListWidget->clear();
    for (const auto& set : allQuizSets) {
        quizSetsListWidget->addItem(QString::fromStdString(set.name));
    }
}

void MainWindow::refreshHomePage() {}
void MainWindow::refreshDeleteList() {
    if (!db) return;
    allQuizSets = db->getAllQuizSets();
    deleteSetsListWidget->clear();
    for (const auto& set : allQuizSets) {
        QListWidgetItem* item = new QListWidgetItem(QString::fromStdString(set.name));
        item->setCheckState(Qt::Unchecked);
        deleteSetsListWidget->addItem(item);
    }
}
void MainWindow::updateQuizDisplay() {}
void MainWindow::showQuizResult() {}

void MainWindow::showErrorDialog(const QString& title, const QString& message) {
    QMessageBox::critical(this, title, message);
}

void MainWindow::showInfoDialog(const QString& title, const QString& message) {
    QMessageBox::information(this, title, message);
}
