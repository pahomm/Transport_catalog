#include <iostream>
#include <iomanip>
#include "json.h"

using namespace std;

namespace Json {

  Document::Document(Node root) : root(move(root)) {
  }

  const Node& Document::GetRoot() const {
    return root;
  }

  Node LoadNode(istream& input);

  Node LoadArray(istream& input) {
    vector<Node> result;

    for (char c; input >> c && c != ']'; ) {
      if (c != ',') {
        input.putback(c);
      }
      result.push_back(LoadNode(input));
    }

    return Node(move(result));
  }
  Node LoadBool(istream& input){
	  string s;
	  input >> s;
	  bool result;
	  while (s.back() != 'e'){
		  input.putback(s.back());
		  s.pop_back();
	  }
	  if (s == "true"){result = true;}
	  else if (s == "false"){result = false;}
	  else {throw runtime_error("cant load bool: " + s);}
	  return Node(result);
  }
  Node LoadDouble(istream& input, int n){
	  input.putback('0');
	  double result;
	  input >> result;
	  if (n < 0) {
		  result *= static_cast<double>(-1);
	  }
	  result += static_cast<double>(n);
	  return Node(result);
  }
  Node LoadInt(istream& input) {
	bool negative = false;
	if (input.peek() == '-'){
		input.ignore();
		negative = true;
	}
    int result = 0;
    while (isdigit(input.peek())) {
      result *= 10;
      result += input.get() - '0';
    }
    if (negative){
    	result *= -1;
    }
    if(input.peek() == '.'){
    	return LoadDouble(input, result);
    }
    return Node(result);
  }

  Node LoadString(istream& input) {
    string line;
    getline(input, line, '"');
    return Node(move(line));
  }

  Node LoadDict(istream& input) {
    map<string, Node> result;

    for (char c; input >> c && c != '}'; ) {
      if (c == ',') {
        input >> c;
      }

      string key = LoadString(input).AsString();
      input >> c;
      result.emplace(move(key), LoadNode(input));
    }

    return Node(move(result));
  }

  Node LoadNode(istream& input) {
    char c;
    input >> c;

    if (c == '[') {
      return LoadArray(input);
    } else if (c == '{') {
      return LoadDict(input);
    } else if (c == '"') {
      return LoadString(input);
    } else if (c == 't' || c == 'f'){
      input.putback(c);
      return LoadBool(input);
    } else {
      input.putback(c);
      return LoadInt(input);
    }
  }

  Document Load(istream& input) {
    return Document{LoadNode(input)};
  }
  void UploadNode(ostream& output, Node node);
  void UploadString(ostream& output, Node node){
	  output << '"' << node.AsString() << '"';
  }
  void UploadInt(ostream& output, Node node){
	  output << node.AsInt();
  }
  void UploadDouble(ostream& output, Node node){
	  output << std::setprecision(6) << node.AsDouble();
  }
  void UploadBool(ostream& output, Node node){
	  if (node.AsBool()) {
		  output << "true";
	  } else {
		  output << "false";
	  }
  }
  void UploadArray(ostream& output, Node node){
	  output << '[';
	  for (auto it = node.AsArray().begin(); it != node.AsArray().end();){
		  UploadNode(output, *it);
		  it++;
		  if (it != node.AsArray().end()){
			  output << ',' << ' ';
		  }
	  }
	  output << ']';
  }
  void UploadMap(ostream& output, Node node){
	  output << '{';
	  for (auto it = node.AsMap().begin(); it != node.AsMap().end();){
		  output << '"' << it->first << '"' << ": ";
		  UploadNode(output, it->second);
		  it++;
		  if (it != node.AsMap().end()){
			  output << ',' << ' ';
		  }
	  }
	  output << '}';
  }

  void UploadNode(ostream& output, Node node){
	  if (std::holds_alternative<string>(node)) {
		  UploadString(output, node);
	  } else if (std::holds_alternative<int>(node)){
		  UploadInt(output, node);
	  } else if (std::holds_alternative<double>(node)){
		  UploadDouble(output, node);
	  } else if (std::holds_alternative<bool>(node)){
		  UploadBool(output, node);
	  } else if (std::holds_alternative<vector<Node>>(node)){
		  UploadArray(output, node);
	  } else if (std::holds_alternative<map<string, Node>>(node)){
		  UploadMap(output, node);
	  }
  }
  void Upload(std::ostream& output, Document doc){
	  UploadNode(output, doc.GetRoot());
  };

}
