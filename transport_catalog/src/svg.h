#pragma once

#include <iostream>
#include <string>
#include <list>
#include <sstream>
#include <optional>
#include <memory>

namespace Svg {

struct Point {
	double x;
	double y;
};

struct Rgb {
	int red;
	int green;
	int blue;
};

class Color {
public:
	Color() : data("none"){}
	Color(string s) : data(move(s)){}
	Color(const char* c) : data(){
		int i = 0;
		while(c[i] != '\0'){
			data += c[i++];
		}
	}
	Color(Rgb rgb) : data("rgb(" + to_string(rgb.red) + ',' + to_string(rgb.green) + ',' + to_string(rgb.blue) + ')'){}
	const string& Get() const {
		return data;
	}
private:
	string data;
};
ostream& operator<<(ostream& out, const Color& color){
	out << color.Get();
	return out;
}

const Color NoneColor;


class Obj{
public:
	virtual string Render() const = 0;
	virtual ~Obj() = default;

protected:
	Color fillColor;
	Color strokeColor;
	double strokeWidth = 1.0;
	optional<string> strokeLineCap;
	optional<string> strokeLineJoin;

};

template<class T>
class Figure : public Obj {
public:
	T& SetFillColor(const Color& c){
		fillColor = c;
		return *static_cast<T*>(this);
	}
	T& SetStrokeColor(const Color& c){
		strokeColor = c;
		return *static_cast<T*>(this);
	}
	T& SetStrokeWidth(double c){
		strokeWidth = c;
		return *static_cast<T*>(this);
	}
	T& SetStrokeLineCap(const string& c){
		strokeLineCap = c;
		return *static_cast<T*>(this);
	}
	T& SetStrokeLineJoin(const string& c) {
		strokeLineJoin = c;
		return *static_cast<T*>(this);
	}
	string VirtRender() const {
		ostringstream res;
		res << "fill=" << '"' << fillColor << '"' << ' ';
		res << "stroke=" << '"' << strokeColor << '"' << ' ';
		res << "stroke-width=" << '"' << strokeWidth << '"' << ' ';
		if (strokeLineCap){
			res << "stroke-linecap=" << '"' << *strokeLineCap << '"' << ' ';
		}
		if (strokeLineJoin){
			res << "stroke-linejoin=" << '"' << *strokeLineJoin << '"' << ' ';
		}
		return res.str();
	}
	virtual string Render() const = 0;
	virtual ~Figure() = default;
};

class Circle : public Figure<Circle> {
public:
	Circle& SetCenter(Point p){
		center = p;
		return *this;
	}
	Circle& SetRadius(double p){
		radius = p;
		return *this;
	}
	string Render() const override {
		ostringstream res;
		res << "<circle ";
		res << "cx=" << '"' << center.x << '"' << ' ' << "cy=" << '"' << center.y << '"' << ' ';
		res << "r=" << '"' << radius << '"' << ' ';
		res << VirtRender();
		res << "/>";
		return res.str();
	}

private:
	Point center = {0,0};
	double radius = 1.0;
};

class Polyline : public Figure<Polyline>{
public:
	Polyline& AddPoint(Point p){
		path.push_back(p);
		return *this;
	}
	string Render() const override {
		ostringstream res;
		res << "<polyline ";
		res << "points=" << '"';
		for (auto point : path){
			res << point.x << ',' << point.y << ' ';
		}
		res << '"' << ' ';
		res << VirtRender();
		res << "/>";
		return res.str();
	}
private:
	list<Point> path;
};

class Text : public Figure<Text>{
public:
	Text& SetPoint(Point p){
		point = p;
		return *this;
	}
	Text& SetOffset(Point p){
		offset = p;
		return *this;
	}
	Text& SetFontSize(uint32_t s){
		fontSize = s;
		return *this;
	}
	Text& SetFontFamily(const string& ff){
		fontFamily = ff;
		return *this;
	}
	Text& SetData(const string& d){
		data = d;
		return *this;
	}
	string Render() const override {
		ostringstream res;
		res << "<text ";
		res << "x=" << '"' << point.x << '"' << ' ' << "y=" << '"' << point.y << '"' << ' ';
		res << "dx=" << '"' << offset.x << '"' << ' ' << "dy=" << '"' << offset.y << '"' << ' ';
		res << "font-size=" << '"' << fontSize << '"' << ' ';
		if (fontFamily) {
			res << "font-family=" << '"' << *fontFamily << '"' << ' ';

		}
		res << VirtRender();
		res << '>' << data << "</text>";
		return res.str();
	}

private:
	Point point = {0,0};
	Point offset = {0,0};
	uint32_t fontSize = 1;
	optional<string> fontFamily;
	string data;
};

class Document {
public:
	Document& AddPtr(unique_ptr<Obj> ptr){
		data.push_back(move(ptr));
		return *this;
	}
	Document& Add(Circle c){
		return AddPtr(make_unique<Circle>(move(c)));
	}
	Document& Add(Polyline p){
		return AddPtr(make_unique<Polyline>(move(p)));
	}
	Document& Add(Text t){
		return AddPtr(make_unique<Text>(move(t)));
	}
	Document& Render(ostream& out){
		out << "<?xml version=" << '"' << "1.0" << '"' << " encoding=" << '"' << "UTF-8" << '"' << " ?>";
		out << "<svg xmlns=" << '"' << "http://www.w3.org/2000/svg" << '"' << " version=" << '"' << "1.1" << '"' << ">";
		for (auto& obj : data) {
			out << obj->Render();
		}
		out << "</svg>";
		return *this;
	}
private:
	list <unique_ptr<Obj>> data;
};

}
