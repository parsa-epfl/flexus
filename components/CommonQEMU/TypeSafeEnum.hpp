#ifndef TYPESAFEENUM_HPP
#define TYPESAFEENUM_HPP

template<typename _ValType>
class enumerator
{
  protected:
    enumerator(const _ValType n)
      : val(n)
    {
    }
    const _ValType val;

  public:
    const _ValType value() const { return val; }

    const bool operator==(const _ValType& a) const { return (val == a.val); }
};

#endif // TYPESAFEENUM_HPP
