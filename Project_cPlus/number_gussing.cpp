#include <iostream>
#include <cstdlib>
#include <ctime>

using namespace std;

int main() {
    int secretNumber, guess, attempts = 0;
    const int MAX_ATTEMPTS = 7;

    // Seed random number generator with current time
    srand(time(0));
    secretNumber = rand() % 100 + 1; // Random number between 1 and 100

    cout << "=============================" << endl;
    cout << "   Welcome to Guessing Game  " << endl;
    cout << "=============================" << endl;
    cout << "I have selected a number between 1 and 100." << endl;
    cout << "You have " << MAX_ATTEMPTS << " tries to guess it!" << endl;

    do {
        cout << "Enter your guess: ";
        cin >> guess;

        attempts++;

        if (guess < secretNumber) {
            cout << "Too low! Try again." << endl;
        } else if (guess > secretNumber) {
            cout << "Too high! Try again." << endl;
        } else {
            cout << "Congratulations! You guessed it in " << attempts << " attempts." << endl;
            break;
        }

        if (attempts >= MAX_ATTEMPTS) {
            cout << "Sorry, you ran out of attempts. The number was " << secretNumber << "." << endl;
            break;
        }

    } while (true);

    cout << "Thanks for playing!" << endl;

    return 0;
}