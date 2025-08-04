#ifndef MANAGER_HPP
#define MANAGER_HPP

#include <_macros/Namespaces.hpp>

GST_BEGIN_NAMESPACE

class Manager final
{
public:
    explicit Manager(int& argc, char**& argv) noexcept;
    ~Manager() noexcept;


protected:


private:
};

GST_END_NAMESPACE

#endif // MANAGER_HPP
