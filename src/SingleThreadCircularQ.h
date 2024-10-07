#pragma once
#include "Container.h"

template <typename T,int BUFFER_SIZE>
class SingleThreadCircularQ
{
    static_assert((BUFFER_SIZE& BUFFER_SIZE - 1) == 0);
    static constexpr int INDEX_MASK = BUFFER_SIZE - 1;
public:
    int Size() const;
    bool Enqueue(const T& data);
    bool Peek(T& data);
    bool Dequeue();

private:
    inline static int GetIndex(int num)
    {
        return num & INDEX_MASK;
    }
    
    /**
     * \brief 데이터가 들어있는 위치
     */
    int _front = 0;
    /**
     * \brief 데이터 삽입할 위치. 데이터는 전까지 있음.
     */
    int _rear = 0;

    
    Array<T,BUFFER_SIZE> _data;
};

template <typename T, int BUFFER_SIZE>
int SingleThreadCircularQ<T, BUFFER_SIZE>::Size() const
{
    const int rear = GetIndex(_rear);
    const int front = GetIndex(_front);

    if (rear >= front)
    {
        return rear - front;
    }

    return BUFFER_SIZE - (front - rear);
}

template <typename T, int BUFFER_SIZE>
bool SingleThreadCircularQ<T, BUFFER_SIZE>::Enqueue(const T& data)
{
    if(GetIndex(_rear+1) == GetIndex(_front))
        return false;
    _data[GetIndex(_rear++)] = data;
    return true;
}

template <typename T, int BUFFER_SIZE>
bool SingleThreadCircularQ<T, BUFFER_SIZE>::Peek(T& data)
{
    if (Size() == 0)
    {
        return false;
    }

    data = _data[GetIndex(_front)];
    return true;
}

template <typename T, int BUFFER_SIZE>
bool SingleThreadCircularQ<T, BUFFER_SIZE>::Dequeue()
{
    if (Size() == 0)
    {
        return false;
    }

    ++_front;
    return true;
}
