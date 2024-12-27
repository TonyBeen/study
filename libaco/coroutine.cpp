/*************************************************************************
    > File Name: coroutine.cpp
    > Author: hsz
    > Brief:
    > Created Time: 2024年12月27日 星期五 11时13分49秒
 ************************************************************************/

#include "coroutine.h"

#include <utils/exception.h>

#include "aco.h"

namespace eular {
struct SharedStackPrivate
{
    aco_share_stack_t *shared_stack = nullptr;
};
    
SharedStack::SharedStack(uint32_t size)
{
    m_p = std::make_shared<SharedStackPrivate>();
    m_p->shared_stack = aco_share_stack_new(size);
}

Coroutine::Coroutine()
{
}

Coroutine::~Coroutine()
{
}

} // namespace eular
