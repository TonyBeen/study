/*************************************************************************
    > File Name: test_shared_ptr.cc
    > Author: hsz
    > Brief:
    > Created Time: Tue 25 Apr 2023 11:20:29 AM CST
 ************************************************************************/

#include <iostream>
#include <memory>

using namespace std;

class Object {
public:
	Object() {
		std::cout << __func__ << "()" << std::endl;
	}

	~Object() {
		std::cout << __func__ << "()" << std::endl;
	}
};

int main(int argc, char **argv)
{
	std::shared_ptr<Object> pObj(new Object[2], std::default_delete<Object[]>());
    return 0;
}
