#ifndef CP_GETAREA_H
#define CP_GETAREA_H

class Shape {
public:
    virtual double getarea() const = 0;
    virtual ~Shape() {}
};

class Circle : public Shape {
public:
    Circle(double r) : radius(r) {}
    double getarea() const override {
        return 3.14159 * radius * radius;
    }

private:
    double radius;
};

class Rectangle : public Shape {
public:
    Rectangle(double w, double h) : width(w), height(h) {}
    double getarea() const override {
        return width * height;
    }

private:
    double width;
    double height;
};

class Square : public Shape {
public:
    Square(double s) : side(s) {}
    double getarea() const override {
        return side * side;
    }

private:
    double side;
};

#endif // CP_GETAREA_H
