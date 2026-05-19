#ifndef STUDENT_H
#define STUDENT_H

#include <QString>
#include <vector>
#include <algorithm>

struct Student {
    QString fullName;
    int recordNumber;
    int grades[5];

    double getAverage() const {
        int sum = 0;
        for (int i = 0; i < 5; ++i) {
            sum += grades[i];
        }
        return static_cast<double>(sum) / 5.0;
    }
};

class StudentManager {
public:
    static void loadFromFile(const QString& filename, std::vector<Student>& students);
    static void saveToFile(const QString& filename, const std::vector<Student>& students);
    
    // Рекурсивная быстрая сортировка по ФИО
    static void sortByNameRecursive(std::vector<Student>& students, int left, int right, bool ascending);
    
    // Нерекурсивная быстрая сортировка по среднему баллу
    static void sortByAverageNonRecursive(std::vector<Student>& students, bool ascending);
};

#endif // STUDENT_H
