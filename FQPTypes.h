#ifndef FQPTYPES_H
#define FQPTYPES_H

#include <memory>
#include <QVariant>

class FQPException
{
public:
    FQPException() {}
    FQPException(const QString& error)  : _error(error) {}

    QString errorString() const {
        return _error;
    }

private:
    QString _error;
};


class FQPUrlException: public FQPException
{
public:
    FQPUrlException(const QString& error) : FQPException(error) {}
};

class FQPTypeException: public FQPException
{
public:
    FQPTypeException() : FQPException("Invalid type conversion") {}
};

class FQPInterpretException: public FQPException
{
public:
    FQPInterpretException() : FQPException("Unable to interpret results") {}
};

class FQPNetworkException: public FQPException
{
public:
    FQPNetworkException() : FQPException("Network error") {}
    FQPNetworkException(const QString& error) : FQPException(error) {}
};

template<typename T> struct FQPDeclarePtrs {
    typedef std::shared_ptr< T >     SharedPtr;
    typedef std::weak_ptr< T >       Ptr;
};

#define FQP_DECLARE_PTRS(type)    \
    class type;                       \
    typedef FQPDeclarePtrs<class type>::Ptr type##Ptr;             \
    typedef FQPDeclarePtrs<class type>::SharedPtr type##SharedPtr;


template <typename TYPE>
TYPE FQPType_GetValueFromMap(const QString& key,
                             const QVariantMap& dict,
                             const TYPE& defaultValue)
{
    QVariantMap::const_iterator it = dict.find(key);
    if ((it != dict.end()) && (it.key() == key) && it->canConvert<TYPE>()) {
        return it->value<TYPE>();
    }
    return defaultValue;
}

template <typename TYPE>
TYPE FQPType_GetValueFromVariant(const QVariant& variant) {
    if (variant.canConvert<TYPE>()) {
        return variant.value<TYPE>();
    } else {
        throw FQPTypeException();
    }
}

#endif // FQPTYPES_H
