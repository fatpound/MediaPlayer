#ifndef MANAGER_HPP
#define MANAGER_HPP

#include <_macros/Namespaces.hpp>

GST_BEGIN_NAMESPACE

class Manager final
{
public:
    explicit Manager(int& argc, char**& argv) noexcept;

    explicit Manager()                   = delete;
    explicit Manager(const Manager&)     = delete;
    explicit Manager(Manager&&) noexcept = delete;

    auto operator = (const Manager&)     -> Manager& = delete;
    auto operator = (Manager&&) noexcept -> Manager& = delete;
    ~Manager() noexcept;


protected:


private:
};

GST_END_NAMESPACE

#endif // MANAGER_HPP
