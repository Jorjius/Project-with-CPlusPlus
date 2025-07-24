#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using namespace std;

struct Student {
    string name;
    int roll;
    float marks;
};

// Function prototypes
void addStudent();
void viewStudents();
void searchStudent();
void deleteStudent();
void showMenu();

int main() {
    int choice;

    cout << "===============================" << endl;
    cout << "      Student Record Manager   " << endl;
    cout << "===============================" << endl;

    do {
        showMenu();
        cout << "Enter your choice: ";
        cin >> choice;
        cin.ignore(); // clear newline

        switch (choice) {
            case 1:
                addStudent();
                break;
            case 2:
                viewStudents();
                break;
            case 3:
                searchStudent();
                break;
            case 4:
                deleteStudent();
                break;
            case 0:
                cout << "Exiting program. Bye!" << endl;
                break;
            default:
                cout << "Invalid choice. Try again!" << endl;
        }

    } while (choice != 0);

    return 0;
}

void showMenu() {
    cout << endl;
    cout << "1. Add Student" << endl;
    cout << "2. View All Students" << endl;
    cout << "3. Search Student by Roll" << endl;
    cout << "4. Delete Student by Roll" << endl;
    cout << "0. Exit" << endl;
}

void addStudent() {
    Student s;
    ofstream fout("students.txt", ios::app);

    if (!fout) {
        cout << "Error opening file!" << endl;
        return;
    }

    cout << "Enter name: ";
    getline(cin, s.name);
    cout << "Enter roll number: ";
    cin >> s.roll;
    cout << "Enter marks: ";
    cin >> s.marks;
    cin.ignore();

    fout << s.name << "," << s.roll << "," << s.marks << endl;
    fout.close();

    cout << "Student added successfully!" << endl;
}

void viewStudents() {
    ifstream fin("students.txt");
    string line;
    int count = 0;

    if (!fin) {
        cout << "No records found." << endl;
        return;
    }

    cout << "Student Records:" << endl;

    while (getline(fin, line)) {
        cout << ++count << ". " << line << endl;
    }

    if (count == 0) {
        cout << "No students yet!" << endl;
    }

    fin.close();
}

void searchStudent() {
    ifstream fin("students.txt");
    string line;
    int rollToSearch;
    bool found = false;

    if (!fin) {
        cout << "No records found." << endl;
        return;
    }

    cout << "Enter roll number to search: ";
    cin >> rollToSearch;
    cin.ignore();

    while (getline(fin, line)) {
        size_t pos1 = line.find(',');
        size_t pos2 = line.rfind(',');

        int roll = stoi(line.substr(pos1 + 1, pos2 - pos1 - 1));

        if (roll == rollToSearch) {
            cout << "Student Found: " << line << endl;
            found = true;
            break;
        }
    }

    if (!found) {
        cout << "Student with roll " << rollToSearch << " not found." << endl;
    }

    fin.close();
}

void deleteStudent() {
    ifstream fin("students.txt");
    ofstream fout("temp.txt");
    string line;
    int rollToDelete;
    bool deleted = false;

    if (!fin) {
        cout << "No records found." << endl;
        return;
    }

    cout << "Enter roll number to delete: ";
    cin >> rollToDelete;
    cin.ignore();

    while (getline(fin, line)) {
        size_t pos1 = line.find(',');
        size_t pos2 = line.rfind(',');

        int roll = stoi(line.substr(pos1 + 1, pos2 - pos1 - 1));

        if (roll == rollToDelete) {
            deleted = true;
            continue; // skip writing this line
        }

        fout << line << endl;
    }

    fin.close();
    fout.close();

    remove("students.txt");
    rename("temp.txt", "students.txt");

    if (deleted) {
        cout << "Student deleted successfully." << endl;
    } else {
        cout << "Student with roll " << rollToDelete << " not found." << endl;
    }
}