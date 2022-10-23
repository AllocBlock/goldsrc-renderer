#pragma once

template <typename T>
class IDataArray
{
public:
    _DEFINE_PTR(IDataArray<T>);

    virtual ~IDataArray() = default;

    bool empty() { return size() == 0; }
    size_t size() { return _sizeV(); }
    void resize(size_t vSize, T vFill = T()) { _resizeV(vSize, vFill); }
    T get(size_t vIndex) { return _getV(vIndex); }
    void set(size_t vIndex, T vData) { _setV(vIndex, vData); }
    void append(T vData, size_t vNum = 1) { _appendV(vData, vNum); }
    void clear() { _clearV(); }
    IDataArray<T>::Ptr copy() { return _copyV(); }

protected:
    virtual size_t _sizeV() = 0;
    virtual void _resizeV(size_t vSize, T vFill) = 0;
    virtual T _getV(size_t vIndex) = 0;
    virtual void _setV(size_t vIndex, T vData) = 0;
    virtual void _appendV(T vData, size_t vNum) = 0;
    virtual void _clearV() = 0;
    virtual IDataArray<T>::Ptr _copyV() = 0;
};
