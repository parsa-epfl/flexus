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

class CategoryHolder {
private:
  std::vector<Category const *> theCategories;

public:
  CategoryHolder(Category const &aCategory) {
    theCategories.push_back(&aCategory);
  }

  CategoryHolder(Category const &aFirstCategory, Category const &aSecondCategory) {
    theCategories.push_back(&aFirstCategory);
    theCategories.push_back(&aSecondCategory);
  }

  CategoryHolder &operator|(CategoryHolder const &aHolder) {
    theCategories.insert(theCategories.end(), aHolder.theCategories.begin(),
                         aHolder.theCategories.end());
    return *this;
  }

  CategoryHolder &operator|(Category const &aCategory) {
    theCategories.push_back(&aCategory);
    return *this;
  }

  typedef std::vector<Category const *>::const_iterator const_iterator;

  const_iterator begin() const {
    return theCategories.begin();
  }
  const_iterator end() const {
    return theCategories.end();
  }
};

class Category {
private:
  std::string theName;
  int32_t theNumber;
  bool theIsDynamic;

public:
  Category(std::string const &aName, bool *aSwitch, bool aIsDynamic = false);

  std::string const &name() const {
    return theName;
  }

  int32_t number() const {
    return theNumber;
  }

  bool isDynamic() const {
    return theIsDynamic;
  }

  bool operator==(Category const &aCategory) {
    return (theNumber == aCategory.theNumber);
  }

  bool operator<(Category const &aCategory) {
    return (theNumber < aCategory.theNumber);
  }

  CategoryHolder operator|(Category const &aCategory) {
    return CategoryHolder(*this, aCategory);
  }
};

class CategoryMgr {
  std::map<std::string, Category *> theCategories;
  int32_t theCatCount;

public:
  CategoryMgr() : theCatCount(0) {
  }
  ~CategoryMgr() {
    std::map<std::string, Category *>::iterator iter = theCategories.begin();
    while (iter != theCategories.end()) {
      if ((*iter).second->isDynamic()) {
        delete (*iter).second;
      }
      ++iter;
    }
  }

  static CategoryMgr &categoryMgr() {
    static CategoryMgr theStaticCategoryMgr;
    return theStaticCategoryMgr;
  }

  Category const &category(std::string const &aCategory) {
    Category *&cat = theCategories[aCategory];
    if (cat == 0) {
      cat = new Category(aCategory, 0, true);
    }
    return *cat;
  }

  int32_t addCategory(Category &aCategory) {
    int32_t cat_num = 0;
    Category *&cat = theCategories[aCategory.name()];
    if (cat != 0) {
      cat_num = cat->number();
    } else {
      cat_num = ++theCatCount;
      cat = &aCategory;
    }
    return cat_num;
  }
};

} // namespace Dbg
} // namespace Flexus

#endif // FLEXUS_CORE_DEBUG_CATEGORY_HPP_INCLUDED
