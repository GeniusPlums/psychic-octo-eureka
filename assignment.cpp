#include <iostream>
#include <string>
#include <queue>
#include <stack>
#include <stdexcept>
#include <iomanip>
#include <limits>
using namespace std;

// Custom exceptions
class ValidationException : public exception {
    string message;
public:
    ValidationException(const string& msg) : message(msg) {}
    const char* what() const noexcept override {
        return message.c_str();
    }
};

class InsufficientFundsException : public exception {
    string message;
public:
    InsufficientFundsException(const string& msg) : message(msg) {}
    const char* what() const noexcept override {
        return message.c_str();
    }
};

class DatabaseException : public exception {
    string message;
public:
    DatabaseException(const string& msg) : message(msg) {}
    const char* what() const noexcept override {
        return message.c_str();
    }
};

// Customer structure
struct Customer {
    string customerId;
    string password;
    string name;
    string email;
    string address;
    string phone;
    double savingsBalance;
    double currentBalance;
    bool isFirstLogin;
};

// Stack for managing default credentials
class DefaultCredentials {
private:
    stack<pair<string, string>> credentials;

public:
    DefaultCredentials() {
        try {
            credentials.push({"CUST010", "PASS010"});
            credentials.push({"CUST009", "PASS009"});
            credentials.push({"CUST008", "PASS008"});
            credentials.push({"CUST007", "PASS007"});
            credentials.push({"CUST006", "PASS006"});
            credentials.push({"CUST005", "PASS005"});
            credentials.push({"CUST004", "PASS004"});
            credentials.push({"CUST003", "PASS003"});
            credentials.push({"CUST002", "PASS002"});
            credentials.push({"CUST001", "PASS001"});
        } catch (...) {
            throw DatabaseException("Failed to initialize default credentials");
        }
    }

    pair<string, string> getNextCredential() {
        if (credentials.empty()) {
            throw DatabaseException("No more default credentials available");
        }
        pair<string, string> cred = credentials.top();
        credentials.pop();
        return cred;
    }
};

// Database management class
class CustomerDatabase {
private:
    static const int MAX_CUSTOMERS = 100;
    Customer customers[MAX_CUSTOMERS][1];
    int customerCount;

public:
    CustomerDatabase() : customerCount(0) {}

    void addCustomer(const Customer& customer) {
        try {
            if (customerCount >= MAX_CUSTOMERS) {
                throw DatabaseException("Maximum customer limit reached");
            }
            if (findCustomer(customer.customerId) != nullptr) {
                throw DatabaseException("Customer ID already exists");
            }
            customers[customerCount][0] = customer;
            customerCount++;
        } catch (const DatabaseException& e) {
            throw;
        } catch (...) {
            throw DatabaseException("Unknown error while adding customer");
        }
    }

    Customer* findCustomer(const string& customerId) {
        try {
            for (int i = 0; i < customerCount; i++) {
                if (customers[i][0].customerId == customerId) {
                    return &customers[i][0];
                }
            }
            return nullptr;
        } catch (...) {
            throw DatabaseException("Error while searching for customer");
        }
    }

    bool validateCredentials(const string& customerId, const string& password) {
        try {
            Customer* customer = findCustomer(customerId);
            return (customer != nullptr && customer->password == password);
        } catch (...) {
            throw DatabaseException("Error while validating credentials");
        }
    }

    bool changePassword(const string& customerId, const string& newPassword) {
        try {
            Customer* customer = findCustomer(customerId);
            if (customer) {
                customer->password = newPassword;
                customer->isFirstLogin = false;
                return true;
            }
            return false;
        } catch (...) {
            throw DatabaseException("Error while changing password");
        }
    }
};

// ATM operations class
class ATM {
private:
    CustomerDatabase& db;
    queue<string> accessQueue;
    const double MIN_SAVINGS_BALANCE = 1000;
    const double MIN_CURRENT_BALANCE = 5000;
    const double SAVINGS_PENALTY = 50;
    const double CURRENT_PENALTY = 250;

public:
    ATM(CustomerDatabase& database) : db(database) {}

    void addToQueue(const string& customerId) {
        try {
            accessQueue.push(customerId);
        } catch (...) {
            throw runtime_error("Error adding customer to queue");
        }
    }

    bool isNextInQueue(const string& customerId) {
        try {
            return !accessQueue.empty() && accessQueue.front() == customerId;
        } catch (...) {
            throw runtime_error("Error checking queue status");
        }
    }

    void removeFromQueue() {
        try {
            if (!accessQueue.empty()) {
                accessQueue.pop();
            }
        } catch (...) {
            throw runtime_error("Error removing customer from queue");
        }
    }

    void checkBalance(const string& customerId) {
        try {
            Customer* customer = db.findCustomer(customerId);
            if (!customer) {
                throw ValidationException("Customer not found");
            }

            cout << "\nAccount Balances for " << customer->name << ":" << endl;
            cout << "Savings Account: Rs. " << fixed << setprecision(2) 
                 << customer->savingsBalance << endl;
            cout << "Current Account: Rs. " << customer->currentBalance << endl;
        } catch (const ValidationException& e) {
            throw;
        } catch (...) {
            throw runtime_error("Error checking balance");
        }
    }

    void withdraw(const string& customerId, char accountType, double amount) {
        try {
            if (amount <= 0) {
                throw ValidationException("Invalid withdrawal amount");
            }

            Customer* customer = db.findCustomer(customerId);
            if (!customer) {
                throw ValidationException("Customer not found");
            }

            double& balance = (accountType == 'S') ? 
                            customer->savingsBalance : customer->currentBalance;
            double minBalance = (accountType == 'S') ? 
                              MIN_SAVINGS_BALANCE : MIN_CURRENT_BALANCE;
            double penalty = (accountType == 'S') ? 
                           SAVINGS_PENALTY : CURRENT_PENALTY;

            if (balance - amount < minBalance) {
                if (balance - amount - penalty < 0) {
                    throw InsufficientFundsException("Insufficient funds for withdrawal");
                }
                balance -= (amount + penalty);
                cout << "Service charge of Rs. " << penalty << " applied" << endl;
            } else {
                balance -= amount;
            }
            
            cout << "Withdrawal successful" << endl;
            checkBalance(customerId);
        } catch (const ValidationException& e) {
            throw;
        } catch (const InsufficientFundsException& e) {
            throw;
        } catch (...) {
            throw runtime_error("Error processing withdrawal");
        }
    }

    void transfer(const string& fromId, const string& toId, 
                 char fromAccountType, char toAccountType, double amount) {
        try {
            if (amount <= 0) {
                throw ValidationException("Invalid transfer amount");
            }

            Customer* fromCustomer = db.findCustomer(fromId);
            Customer* toCustomer = db.findCustomer(toId);

            if (!fromCustomer || !toCustomer) {
                throw ValidationException("Invalid customer ID(s)");
            }

            double& fromBalance = (fromAccountType == 'S') ? 
                                fromCustomer->savingsBalance : fromCustomer->currentBalance;
            double& toBalance = (toAccountType == 'S') ? 
                              toCustomer->savingsBalance : toCustomer->currentBalance;
            
            double minBalance = (fromAccountType == 'S') ? 
                              MIN_SAVINGS_BALANCE : MIN_CURRENT_BALANCE;
            double penalty = (fromAccountType == 'S') ? 
                           SAVINGS_PENALTY : CURRENT_PENALTY;

            if (fromBalance - amount < minBalance) {
                if (fromBalance - amount - penalty < 0) {
                    throw InsufficientFundsException("Insufficient funds for transfer");
                }
                fromBalance -= (amount + penalty);
                cout << "Service charge of Rs. " << penalty << " applied" << endl;
            } else {
                fromBalance -= amount;
            }

            toBalance += amount;
            cout << "Transfer successful" << endl;
            checkBalance(fromId);
        } catch (const ValidationException& e) {
            throw;
        } catch (const InsufficientFundsException& e) {
            throw;
        } catch (...) {
            throw runtime_error("Error processing transfer");
        }
    }
};

// Main application class
class BankApplication {
private:
    CustomerDatabase db;
    DefaultCredentials defaultCreds;
    ATM atm;
    string currentUserId;

    bool isEmptyOrWhitespace(const string& str) {
        return str.empty() || str.find_first_not_of(" \t\n\r") == string::npos;
    }

    string getValidInput(const string& prompt, bool allowSpaces = true) {
        string input;
        bool valid = false;
        
        do {
            try {
                cout << prompt;
                if (allowSpaces) {
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                    getline(cin, input);
                } else {
                    cin >> input;
                    cin.ignore(numeric_limits<streamsize>::max(), '\n');
                }

                if (isEmptyOrWhitespace(input)) {
                    throw ValidationException("Input cannot be empty or only whitespace");
                }
                valid = true;
            } catch (const ValidationException& e) {
                cout << "Error: " << e.what() << ". Please try again." << endl;
            } catch (...) {
                cout << "Error reading input. Please try again." << endl;
            }
        } while (!valid);

        return input;
    }

public:
    BankApplication() : atm(db), currentUserId("") {}

    void run() {
        while (true) {
            try {
                cout << "\n=== Bank ATM System ===" << endl;
                cout << "1. Login" << endl;
                cout << "2. Sign Up" << endl;
                cout << "3. Exit" << endl;
                cout << "Choose an option: ";

                string choice = getValidInput("", false);
                
                switch (stoi(choice)) {
                    case 1:
                        login();
                        break;
                    case 2:
                        signUp();
                        break;
                    case 3:
                        cout << "Thank you for using our services!" << endl;
                        return;
                    default:
                        throw ValidationException("Invalid option");
                }
            } catch (const ValidationException& e) {
                cout << "Error: " << e.what() << endl;
            } catch (const exception& e) {
                cout << "An error occurred: " << e.what() << endl;
            } catch (...) {
                cout << "An unexpected error occurred" << endl;
            }
        }
    }

private:
    void login() {
        try {
            cout << "\n=== Login ===" << endl;
            string customerId = getValidInput("Enter Customer ID: ", false);
            string password = getValidInput("Enter Password: ", false);

            if (db.validateCredentials(customerId, password)) {
                currentUserId = customerId;
                atm.addToQueue(currentUserId);
                cout << "Login successful!" << endl;
                
                Customer* customer = db.findCustomer(customerId);
                if (customer && customer->isFirstLogin) {
                    cout << "\nThis is your first login. You must change your password." << endl;
                    changePassword(true);
                }
                
                showMainMenu();
            } else {
                throw ValidationException("Invalid credentials");
            }
        } catch (const ValidationException& e) {
            cout << "Login failed: " << e.what() << endl;
        } catch (...) {
            cout << "An error occurred during login" << endl;
        }
    }

    void signUp() {
        try {
            Customer newCustomer;
            cout << "\n=== New Customer Registration ===" << endl;
            
            newCustomer.name = getValidInput("Enter Name: ");
            if (newCustomer.name.length() < 2) {
                throw ValidationException("Name must be at least 2 characters long");
            }

            newCustomer.email = getValidInput("Enter Email: ");
            if (newCustomer.email.find('@') == string::npos || 
                newCustomer.email.find('.') == string::npos) {
                throw ValidationException("Invalid email format");
            }

            newCustomer.address = getValidInput("Enter Address: ");
            if (newCustomer.address.length() < 5) {
                throw ValidationException("Address must be at least 5 characters long");
            }

            newCustomer.phone = getValidInput("Enter Phone: ", false);
            if (newCustomer.phone.length() != 10 || 
                !all_of(newCustomer.phone.begin(), newCustomer.phone.end(), ::isdigit)) {
                throw ValidationException("Phone number must be exactly 10 digits");
            }

            newCustomer.savingsBalance = 10000;
            newCustomer.currentBalance = 25000;
            newCustomer.isFirstLogin = true;

            pair<string, string> defaultCred = defaultCreds.getNextCredential();
            newCustomer.customerId = defaultCred.first;
            newCustomer.password = defaultCred.second;

            db.addCustomer(newCustomer);

            cout << "\nRegistration successful!" << endl;
            cout << "Your assigned credentials:" << endl;
            cout << "Customer ID: " << newCustomer.customerId << endl;
            cout << "Default Password: " << newCustomer.password << endl;
            cout << "\nYou will be required to change your password upon first login." << endl;

        } catch (const ValidationException& e) {
            cout << "Registration failed: " << e.what() << endl;
        } catch (const DatabaseException& e) {
            cout << "Database error: " << e.what() << endl;
        } catch (...) {
            cout << "An unexpected error occurred during registration" << endl;
        }
    }
void changePassword(bool isFirstTime = false) {
        try {
            string newPassword, confirmPassword;
            bool valid = false;
            
            do {
                newPassword = getValidInput("Enter new password: ", false);
                
                if (newPassword.length() < 6) {
                    throw ValidationException("Password must be at least 6 characters long");
                }

                confirmPassword = getValidInput("Confirm new password: ", false);

                if (newPassword != confirmPassword) {
                    throw ValidationException("Passwords do not match");
                }
                valid = true;
            } while (!valid);

            if (!db.changePassword(currentUserId, newPassword)) {
                throw DatabaseException("Failed to update password");
            }
            cout << "Password changed successfully!" << endl;
        } catch (const ValidationException& e) {
            cout << "Password change failed: " << e.what() << endl;
            if (isFirstTime) {
                cout << "You must change your password before continuing. Please try again." << endl;
                changePassword(true);
            }
        } catch (const DatabaseException& e) {
            cout << "Database error: " << e.what() << endl;
        } catch (...) {
            cout << "An unexpected error occurred while changing password" << endl;
        }
    }

    void showMainMenu() {
        while (true && atm.isNextInQueue(currentUserId)) {
            try {
                cout << "\n=== Main Menu ===" << endl;
                cout << "1. Check Balance" << endl;
                cout << "2. Withdraw" << endl;
                cout << "3. Transfer" << endl;
                cout << "4. Change Password" << endl;
                cout << "5. Logout" << endl;
                cout << "Choose an option: ";

                string choice = getValidInput("", false);
                int option = stoi(choice);

                switch (option) {
                    case 1:
                        atm.checkBalance(currentUserId);
                        break;
                    case 2:
                        handleWithdrawal();
                        break;
                    case 3:
                        handleTransfer();
                        break;
                    case 4:
                        changePassword();
                        break;
                    case 5:
                        logout();
                        return;
                    default:
                        throw ValidationException("Invalid option");
                }
            } catch (const ValidationException& e) {
                cout << "Error: " << e.what() << endl;
            } catch (const exception& e) {
                cout << "An error occurred: " << e.what() << endl;
            } catch (...) {
                cout << "An unexpected error occurred" << endl;
            }
        }
    }

    void handleWithdrawal() {
        try {
            char accountType;
            double amount;

            do {
                string input = getValidInput("Select account (S for Savings, C for Current): ", false);
                accountType = toupper(input[0]);
                
                if (accountType != 'S' && accountType != 'C') {
                    throw ValidationException("Invalid account type. Please enter S or C");
                }
            } while (accountType != 'S' && accountType != 'C');

            string amountStr = getValidInput("Enter amount to withdraw: ", false);
            try {
                amount = stod(amountStr);
                if (amount <= 0) {
                    throw ValidationException("Amount must be greater than zero");
                }
            } catch (const invalid_argument&) {
                throw ValidationException("Invalid amount format");
            }

            atm.withdraw(currentUserId, accountType, amount);

        } catch (const ValidationException& e) {
            cout << "Withdrawal failed: " << e.what() << endl;
        } catch (const InsufficientFundsException& e) {
            cout << "Withdrawal failed: " << e.what() << endl;
        } catch (...) {
            cout << "An unexpected error occurred during withdrawal" << endl;
        }
    }

    void handleTransfer() {
        try {
            cout << "\nTransfer Options:" << endl;
            cout << "1. Between own accounts" << endl;
            cout << "2. To another customer" << endl;
            
            string choiceStr = getValidInput("Select option: ", false);
            int choice = stoi(choiceStr);

            if (choice != 1 && choice != 2) {
                throw ValidationException("Invalid transfer option");
            }

            char fromAccount, toAccount;
            string toCustomerId;
            double amount;

            // Get source account type
            do {
                string input = getValidInput("From account (S/C): ", false);
                fromAccount = toupper(input[0]);
                if (fromAccount != 'S' && fromAccount != 'C') {
                    throw ValidationException("Invalid source account type");
                }
            } while (fromAccount != 'S' && fromAccount != 'C');

            if (choice == 1) {
                toCustomerId = currentUserId;
                do {
                    string input = getValidInput("To account (S/C): ", false);
                    toAccount = toupper(input[0]);
                    if (toAccount != 'S' && toAccount != 'C') {
                        throw ValidationException("Invalid destination account type");
                    }
                } while (toAccount != 'S' && toAccount != 'C');
            } else {
                toCustomerId = getValidInput("Enter recipient's Customer ID: ", false);
                do {
                    string input = getValidInput("To recipient's account (S/C): ", false);
                    toAccount = toupper(input[0]);
                    if (toAccount != 'S' && toAccount != 'C') {
                        throw ValidationException("Invalid destination account type");
                    }
                } while (toAccount != 'S' && toAccount != 'C');
            }

            string amountStr = getValidInput("Enter amount to transfer: ", false);
            try {
                amount = stod(amountStr);
                if (amount <= 0) {
                    throw ValidationException("Amount must be greater than zero");
                }
            } catch (const invalid_argument&) {
                throw ValidationException("Invalid amount format");
            }

            atm.transfer(currentUserId, toCustomerId, fromAccount, toAccount, amount);

        } catch (const ValidationException& e) {
            cout << "Transfer failed: " << e.what() << endl;
        } catch (const InsufficientFundsException& e) {
            cout << "Transfer failed: " << e.what() << endl;
        } catch (const DatabaseException& e) {
            cout << "Database error: " << e.what() << endl;
        } catch (...) {
            cout << "An unexpected error occurred during transfer" << endl;
        }
    }

    void logout() {
        try {
            atm.removeFromQueue();
            currentUserId = "";
            cout << "Logged out successfully" << endl;
        } catch (...) {
            cout << "Error during logout" << endl;
        }
    }
};

int main() {
    try {
        BankApplication app;
        app.run();
    } catch (const exception& e) {
        cout << "Fatal error: " << e.what() << endl;
        return 1;
    } catch (...) {
        cout << "Fatal error: Unknown exception occurred" << endl;
        return 1;
    }
    return 0;
}
        
