#ifndef QUIZ_ENGINE_H
#define QUIZ_ENGINE_H

#include "models.h"
#include <random>
#include <vector>
#include <numeric>
#include <algorithm>
#include <stdexcept>

class QuizEngine {
public:
    QuizEngine();

    // Quiz o'yni bilan ishlash
    void startQuiz(const QuizSet& set);
    void reset();

    bool isActive() const { return active; }
    bool hasMore() const;

    Question getCurrentQuestion() const;
    std::vector<Answer> getShuffledAnswers();

    // Returns true if correct
    bool submitAnswer(int answerId);

    QuizResult getResult() const;

    // Status
    int getCurrentIndex() const { return currentPos; }
    int getTotalQuestions() const { return static_cast<int>(shuffledIndices.size()); }
    int getCurrentScore() const { return currentScore; }
    double getCurrentPercentage() const;

private:
    QuizSet activeSet;
    std::vector<int> shuffledIndices;
    std::vector<Answer> currentAnswers;
    int currentPos;
    int currentScore;
    bool active;
    std::mt19937 rng;

    void shuffleCurrentAnswers();
};

#endif // QUIZ_ENGINE_H
