#include <iostream>
#include <vector>
#include <string>

using namespace std;

// Simple structure to hold a task
struct Task {
    string description;
    bool isDone;
};

// Function prototypes
void showMenu();
void addTask(vector<Task> &tasks);
void viewTasks(const vector<Task> &tasks);
void deleteTask(vector<Task> &tasks);

int main() {
    vector<Task> tasks;
    int choice;

    cout << "===============================" << endl;
    cout << "     Simple To-Do List App     " << endl;
    cout << "===============================" << endl;

    do {
        showMenu();
        cout << "Enter your choice: ";
        cin >> choice;

        // To handle input buffer issues
        cin.ignore();

        switch (choice) {
            case 1:
                addTask(tasks);
                break;
            case 2:
                viewTasks(tasks);
                break;
            case 3:
                deleteTask(tasks);
                break;
            case 0:
                cout << "Goodbye! Have a productive day!" << endl;
                break;
            default:
                cout << "Invalid choice. Please try again." << endl;
        }

    } while (choice != 0);

    return 0;
}

void showMenu() {
    cout << endl;
    cout << "------- MENU -------" << endl;
    cout << "1. Add Task" << endl;
    cout << "2. View Tasks" << endl;
    cout << "3. Delete Task" << endl;
    cout << "0. Exit" << endl;
    cout << "---------------------" << endl;
}

void addTask(vector<Task> &tasks) {
    Task newTask;
    cout << "Enter task description: ";
    getline(cin, newTask.description);
    newTask.isDone = false;
    tasks.push_back(newTask);
    cout << "Task added successfully!" << endl;
}

void viewTasks(const vector<Task> &tasks) {
    if (tasks.empty()) {
        cout << "No tasks yet. Add some!" << endl;
        return;
    }

    cout << "Your Tasks:" << endl;
    for (size_t i = 0; i < tasks.size(); ++i) {
        cout << i + 1 << ". " << tasks[i].description;
        if (tasks[i].isDone) {
            cout << " [Done]";
        }
        cout << endl;
    }
}

void deleteTask(vector<Task> &tasks) {
    if (tasks.empty()) {
        cout << "No tasks to delete!" << endl;
        return;
    }

    int taskNum;
    viewTasks(tasks);
    cout << "Enter the task number to delete: ";
    cin >> taskNum;

    if (taskNum < 1 || taskNum > tasks.size()) {
        cout << "Invalid task number!" << endl;
    } else {
        tasks.erase(tasks.begin() + taskNum - 1);
        cout << "Task deleted successfully." << endl;
    }

    // Clear input buffer
    cin.ignore();
}