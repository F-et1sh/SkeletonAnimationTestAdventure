#pragma once

class Model {
public:
    struct Primitive {
    public:
        int  m_someInteger        = 0;
        int* m_someIntegerPointer = 0;
    };

    Model()  = default;
    ~Model() = default;

private:
    int  m_someInteger        = 0;
    int* m_someIntegerPointer = 0;
};
