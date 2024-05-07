/*************************************************************************
    > File Name: test_std_forword.cc
    > Author: hsz
    > Brief:
    > Created Time: 2024年05月07日 星期二 13时38分44秒
 ************************************************************************/

#include <iostream>
#include <utility>

class ClassTest {
public:
    ClassTest(bool &isDeconstructionCalled) :
        refIsDeconstructionCalled(isDeconstructionCalled)
    {
        isDeconstructionCalled = false;
    }

    ~ClassTest() { refIsDeconstructionCalled = true; }

    bool &refIsDeconstructionCalled;
};

template<typename... Args>
void forward_args(Args&&... args) {
    auto ptr = new ClassTest(std::forward<Args>(args)...);
    delete ptr;
}

int main() {
    bool x = false;

    forward_args(x);
    printf("x = %d\n", x);

    return 0;
}
