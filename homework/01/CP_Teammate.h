#ifndef CP_TEAMMATE_H
#define CP_TEAMMATE_H

#include <iostream>
#include <string>
using namespace std;

class Member
{
public:
    Member() : m_name("?"), m_age(0) {}

    Member(const string name, const int age) : m_name(name), m_age(age) {}
    
    string getName() const
    {
        return m_name;
    }
    int getAge() const
    {
        return m_age;
    }

private:
    const string m_name;
    const int m_age;
};

ostream& operator<<(ostream& os, const Member& member)
{
    os << "Name: " << member.getName() << ", Age: " << member.getAge();
    return os;
}

class MemberList
{
public:
    MemberList(Member* members, int size) : m_members(members), m_size(size) {}

    int operator[](const string& name) const
    {
        for (int i = 0; i < m_size; i++)
        {
            if (m_members[i].getName() == name)
            {
                return m_members[i].getAge();
            }
        }
        return -1;
    }

private:
    Member* m_members;
    int m_size;
};

#endif // CP_TEAMMATE_H
