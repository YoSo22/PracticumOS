#include "student.h"
#include <QFile>
#include <QTextStream>
#include <QStack>

void StudentManager::loadFromFile(const QString& filename, std::vector<Student>& students) {
    students.clear();
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }

    QTextStream in(&file);
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.isEmpty()) continue;

        QStringList parts = line.split(';');
        if (parts.size() >= 7) {
            Student s;
            s.fullName = parts[0];
            s.recordNumber = parts[1].toInt();
            for (int i = 0; i < 5; ++i) {
                s.grades[i] = parts[i + 2].toInt();
            }
            students.push_back(s);
        }
    }
    file.close();
}

void StudentManager::saveToFile(const QString& filename, const std::vector<Student>& students) {
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }

    QTextStream out(&file);
    for (const auto& s : students) {
        out << s.fullName << ";" 
            << s.recordNumber << ";";
        for (int i = 0; i < 5; ++i) {
            out << s.grades[i];
            if (i < 4) out << ";";
        }
        out << "\n";
    }
    file.close();
}

// Рекурсивная быстрая сортировка по ФИО
void StudentManager::sortByNameRecursive(std::vector<Student>& students, int left, int right, bool ascending) {
    if (left >= right) return;

    int i = left, j = right;
    QString pivot = students[(left + right) / 2].fullName;

    while (i <= j) {
        while ((ascending && students[i].fullName < pivot) || 
               (!ascending && students[i].fullName > pivot)) {
            i++;
        }
        while ((ascending && students[j].fullName > pivot) || 
               (!ascending && students[j].fullName < pivot)) {
            j--;
        }
        if (i <= j) {
            std::swap(students[i], students[j]);
            i++;
            j--;
        }
    }

    sortByNameRecursive(students, left, j, ascending);
    sortByNameRecursive(students, i, right, ascending);
}

// Нерекурсивная быстрая сортировка по среднему баллу (со стеком)
void StudentManager::sortByAverageNonRecursive(std::vector<Student>& students, bool ascending) {
    if (students.empty()) return;

    std::stack<std::pair<int, int>> stack;
    stack.push({0, static_cast<int>(students.size()) - 1});

    while (!stack.empty()) {
        int left = stack.top().first;
        int right = stack.top().second;
        stack.pop();

        if (left >= right) continue;

        int i = left, j = right;
        double pivot = students[(left + right) / 2].getAverage();

        while (i <= j) {
            while ((ascending && students[i].getAverage() < pivot) || 
                   (!ascending && students[i].getAverage() > pivot)) {
                i++;
            }
            while ((ascending && students[j].getAverage() > pivot) || 
                   (!ascending && students[j].getAverage() < pivot)) {
                j--;
            }
            if (i <= j) {
                std::swap(students[i], students[j]);
                i++;
                j--;
            }
        }

        if (j > left) stack.push({left, j});
        if (i < right) stack.push({i, right});
    }
}
