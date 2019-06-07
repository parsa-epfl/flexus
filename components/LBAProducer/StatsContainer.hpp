#ifndef _STATCONTAINER_H_DEF_
#define _STATCONTAINER_H_DEF_

#include <vector>

template< typename T >
class StatsContainer {
    private:
        std::vector< T > underlyingContainer;

    public:
        void append(T& appendMe);
        T& get(size_t idx);
        void writeToFile(std::string fname);
        void readFromFile(std::string fname, size_t num_elements);
        size_t size() { return underlyingContainer.size(); }
};

#include "StatsContainer.tpp"

#endif // ifndef _STATCONATINER_H_DEF_
