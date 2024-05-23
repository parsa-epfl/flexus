//  DO-NOT-REMOVE begin-copyright-block
// QFlex consists of several software components that are governed by various
// licensing terms, in addition to software that was developed internally.
// Anyone interested in using QFlex needs to fully understand and abide by the
// licenses governing all the software components.
//
// ### Software developed externally (not by the QFlex group)
//
//     * [NS-3] (https://www.gnu.org/copyleft/gpl.html)
//     * [QEMU] (http://wiki.qemu.org/License)
//     * [SimFlex] (http://parsa.epfl.ch/simflex/)
//     * [GNU PTH] (https://www.gnu.org/software/pth/)
//
// ### Software developed internally (by the QFlex group)
// **QFlex License**
//
// QFlex
// Copyright (c) 2020, Parallel Systems Architecture Lab, EPFL
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright notice,
//       this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above copyright notice,
//       this list of conditions and the following disclaimer in the documentation
//       and/or other materials provided with the distribution.
//     * Neither the name of the Parallel Systems Architecture Laboratory, EPFL,
//       nor the names of its contributors may be used to endorse or promote
//       products derived from this software without specific prior written
//       permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE PARALLEL SYSTEMS ARCHITECTURE LABORATORY,
// EPFL BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
// GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF
// THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//  DO-NOT-REMOVE end-copyright-block
#ifndef FLEXUS_CORE_DEBUG_CATEGORY_HPP_INCLUDED
#define FLEXUS_CORE_DEBUG_CATEGORY_HPP_INCLUDED

#include <map>
#include <string>
#include <utility>
#include <vector>

namespace Flexus {
namespace Dbg {

using namespace std::rel_ops;

class Category;

class CategoryHolder
{
  private:
    std::vector<Category const*> theCategories;

  public:
    CategoryHolder(Category const& aCategory) { theCategories.push_back(&aCategory); }

    CategoryHolder(Category const& aFirstCategory, Category const& aSecondCategory)
    {
        theCategories.push_back(&aFirstCategory);
        theCategories.push_back(&aSecondCategory);
    }

    CategoryHolder& operator|(CategoryHolder const& aHolder)
    {
        theCategories.insert(theCategories.end(), aHolder.theCategories.begin(), aHolder.theCategories.end());
        return *this;
    }

    CategoryHolder& operator|(Category const& aCategory)
    {
        theCategories.push_back(&aCategory);
        return *this;
    }

    typedef std::vector<Category const*>::const_iterator const_iterator;

    const_iterator begin() const { return theCategories.begin(); }
    const_iterator end() const { return theCategories.end(); }
};

class Category
{
  private:
    std::string theName;
    int32_t theNumber;
    bool theIsDynamic;

  public:
    Category(std::string const& aName, bool* aSwitch, bool aIsDynamic = false);

    std::string const& name() const { return theName; }

    int32_t number() const { return theNumber; }

    bool isDynamic() const { return theIsDynamic; }

    bool operator==(Category const& aCategory) { return (theNumber == aCategory.theNumber); }

    bool operator<(Category const& aCategory) { return (theNumber < aCategory.theNumber); }

    CategoryHolder operator|(Category const& aCategory) { return CategoryHolder(*this, aCategory); }
};

class CategoryMgr
{
    std::map<std::string, Category*> theCategories;
    int32_t theCatCount;

  public:
    CategoryMgr()
      : theCatCount(0)
    {
    }
    ~CategoryMgr()
    {
        std::map<std::string, Category*>::iterator iter = theCategories.begin();
        while (iter != theCategories.end()) {
            if ((*iter).second->isDynamic()) { delete (*iter).second; }
            ++iter;
        }
    }

    static CategoryMgr& categoryMgr()
    {
        static CategoryMgr theStaticCategoryMgr;
        return theStaticCategoryMgr;
    }

    Category const& category(std::string const& aCategory)
    {
        Category*& cat = theCategories[aCategory];
        if (cat == 0) { cat = new Category(aCategory, 0, true); }
        return *cat;
    }

    int32_t addCategory(Category& aCategory)
    {
        int32_t cat_num = 0;
        Category*& cat  = theCategories[aCategory.name()];
        if (cat != 0) {
            cat_num = cat->number();
        } else {
            cat_num = ++theCatCount;
            cat     = &aCategory;
        }
        return cat_num;
    }
};

} // namespace Dbg
} // namespace Flexus

#endif // FLEXUS_CORE_DEBUG_CATEGORY_HPP_INCLUDED
