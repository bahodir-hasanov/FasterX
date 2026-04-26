#include "quiz_engine.h"
#include <chrono>

QuizEngine::QuizEngine() 
    : currentPos(0), currentScore(0), active(false) {
    auto seed = std::chrono::steady_clock::now().time_since_epoch().count();
    rng = std::mt19937((unsigned long)seed);
}

void QuizEngine::startQuiz(const QuizSet& set) {
    if (set.questions.empty())
        throw std::runtime_error("Quiz bo'sh! Kamida 1 ta savol bo'lishi kerak.");
    
    activeSet = set;
    currentPos = 0;
    currentScore = 0;
    active = true;
    
    shuffledIndices.resize(set.questions.size());
    std::iota(shuffledIndices.begin(), shuffledIndices.end(), 0);
    std::shuffle(shuffledIndices.begin(), shuffledIndices.end(), rng);
    
    shuffleCurrentAnswers();
}

void QuizEngine::reset() {
    active = false;
    currentPos = 0;
    currentScore = 0;
    shuffledIndices.clear();
    currentAnswers.clear();
}

bool QuizEngine::hasMore() const {
    return active && currentPos < (int)shuffledIndices.size();
}

Question QuizEngine::getCurrentQuestion() const {
    if (!active || currentPos >= (int)shuffledIndices.size())
        throw std::out_of_range("Savol mavjud emas");
    return activeSet.questions[shuffledIndices[currentPos]];
}

std::vector<Answer> QuizEngine::getShuffledAnswers() {
    return currentAnswers;
}

void QuizEngine::shuffleCurrentAnswers() {
    if (currentPos < (int)shuffledIndices.size()) {
        currentAnswers = activeSet.questions[shuffledIndices[currentPos]].answers;
        std::shuffle(currentAnswers.begin(), currentAnswers.end(), rng);
    }
}

bool QuizEngine::submitAnswer(int answerId) {
    if (!active || currentPos >= (int)shuffledIndices.size())
        return false;
    
    const auto& q = activeSet.questions[shuffledIndices[currentPos]];
    bool correct = false;
    for (const auto& a : q.answers) {
        if (a.id == answerId && a.isCorrect) {
            correct = true;
            break;
        }
    }
    
    if (correct) currentScore++;
    currentPos++;
    
    if (hasMore()) shuffleCurrentAnswers();
    else active = false;
    
    return correct;
}

double QuizEngine::getCurrentPercentage() const {
    int total = (int)shuffledIndices.size();
    if (total == 0) return 0.0;
    return (currentScore * 100.0) / total;
}

QuizResult QuizEngine::getResult() const {
    QuizResult r;
    r.quizSetId = activeSet.id;
    r.quizSetName = activeSet.name;
    r.score = currentScore;
    r.total = (int)shuffledIndices.size();
    r.percentage = (r.total > 0) ? (currentScore * 100.0 / r.total) : 0.0;
    r.completedAt = time(nullptr);
    return r;
}