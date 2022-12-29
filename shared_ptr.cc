/*************************************************************************
    > File Name: shared_ptr.cc
    > Author: hsz
    > Brief:
    > Created Time: Wed 27 Oct 2021 10:47:43 AM CST
 ************************************************************************/

#include <iostream>
#include <memory>

using namespace std;

std::weak_ptr<int> gWeakPtr;

void useWeakPtr()
{
    if (auto sp = gWeakPtr.lock()) {
        cout << *sp << endl;
    } else {
        cout << "sp is null\n";
    }
}

void test()
{
    {
        // shared_ptr失效后，weak_ptr也不能在使用
        std::shared_ptr<int> sp(new int(10));
        gWeakPtr = sp;
        useWeakPtr();   // 10
    }
    useWeakPtr();   // sp is null
    // 从输出可以看出，weak_ptr只能在shared_ptr有效期内使用
}

int test_weak_ptr2()
{
	// OLD, problem with dangling pointer
	// PROBLEM: ref will point to undefined data!
	int* ptr = new int(10);
	int* ref = ptr;
	delete ptr;
 
	// NEW
	// SOLUTION: check expired() or lock() to determine if pointer is valid
	// empty definition
	std::shared_ptr<int> sptr;
 
	// takes ownership of pointer
	sptr.reset(new int);
	*sptr = 10;
 
	// get pointer to data without taking ownership
	std::weak_ptr<int> weak1 = sptr;
 
	// deletes managed object, acquires new pointer
	sptr.reset(new int);
	*sptr = 5;
 
	// get pointer to new data without taking ownership
	std::weak_ptr<int> weak2 = sptr;
 
	// weak1 is expired!
 
	if (auto tmp = weak1.lock())
		std::cout << *tmp << '\n';
	else
		std::cout << "weak1 is expired\n";
 
	// weak2 points to new data (5)
 
	if (auto tmp = weak2.lock())
		std::cout << *tmp << '\n';
	else
		std::cout << "weak2 is expired\n";
 
	return 0;
}

int main(int argc, char **argv)
{
    test();
    cout << "--------------------------\n";
    test_weak_ptr2();
    return 0;
}
