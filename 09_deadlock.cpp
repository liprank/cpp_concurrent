#include <mutex>
#include <string>
#include <set>
#include <iostream>
#include <thread>

using namespace std;

class Account
{
public:
    Account(string name, double money)
    :name_(name),
    money_(money)
    {

    };

    void changeMoney(double amount)
    {
        money_ += amount;
    }

    string getName() const
    {
        return name_;
    }

    double getMoney() const
    {
        return money_;
    }

    mutex* getMutex()
    {
        return &lock_;
    }
private:
    string name_;
    double money_;
    mutex lock_;
};

class Bank
{
public:
    void addAccount(Account* account)
    {
        accounts_.insert(account);
    }

    // 转移金额的操作非常容易死锁，因为涉及到两个账户的锁定
    bool transfer(Account* from, Account* to, double amount)
    {
        if (from == to)
        {
            return false;
        }

        // 先锁定第一个账户
        unique_lock<mutex> lock1(*from->getMutex());
        // 再锁定第二个账户
        unique_lock<mutex> lock2(*to->getMutex());

        if (from->getMoney() < amount)
        {
            return false;
        }

        from->changeMoney(-amount);
        to->changeMoney(amount);
        return true;
    }

    double totalMoney() const
    {
        double total = 0.0;
        for (const auto& account : accounts_)
        {
            total += account->getMoney();
        }
        return total;
    }

private:
    set<Account*> accounts_;

};

void randomTransfer(Bank* bank, Account* account1, Account* account2)
{
    for (int i = 0; i < 10000; ++i)
    {
        double amount = rand() % 100;
        if(bank->transfer(account1, account2, amount))
        {
            cout << "Transferred " << amount << " from " << account1->getName() << " to " << account2->getName();
            cout << " Total money in bank: " << bank->totalMoney() << endl;
        }
        else{
            cout << "Failed to transfer " << amount << " from " << account1->getName() << " to " << account2->getName() << endl;
            cout << " required money: " << amount << ", current money: " << account1->getMoney() << endl;
        }
    }
}

int main()
{
    Account a("AccountA", 10000);
    Account b("AccountB", 10000);
    Bank bank;
    bank.addAccount(&a);
    bank.addAccount(&b);
    thread t1(randomTransfer, &bank, &a, &b);
    thread t2(randomTransfer, &bank, &b, &a);
    t1.join();
    t2.join();
    cout << "Final total money in bank: " << bank.totalMoney() << endl;

    return 0;
}